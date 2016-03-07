#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <copyinout.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <vnode.h>
#include <synch.h>
#include <proc_syscall.h>
#include <kern/stat.h>
#include <file_syscall.h>
#include <thread.h>
#include <mips/trapframe.h>
#include <kern/wait.h>




void 
entrypoint(void* tf,unsigned long junk)
{
	junk ++;
	
	struct trapframe kernel_tf;
	kernel_tf = *(struct trapframe *)tf;

	kernel_tf.tf_v0=0;
	kernel_tf.tf_a3=0;
	kernel_tf.tf_epc+=4;

	kfree(tf); // remove trap frame from kernel heap.
	as_activate();
	mips_usermode(&kernel_tf); // fallback to user mode	

}

int 
sys_fork(struct trapframe* tf, int *retval)
{


	struct proc *child_proc = proc_create_runprogram("child_process");
	if(child_proc==NULL)
		return ENOMEM;
	
	as_copy(curproc->p_addrspace, &child_proc->p_addrspace);


	for( int i=0; i < OPEN_MAX; i++) {
		if( curproc->p_fdtable[i] != NULL  ) {
			curproc -> p_fdtable[i] -> reference_count ++;
			child_proc -> p_fdtable[i] = curproc -> p_fdtable[i];
		}
	}
	
	struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
	if(child_tf == NULL)
		return ENOMEM;



	*child_tf = *tf;
	child_proc->ppid = curproc->pid; // set parent pid

	*retval = child_proc->pid;

	thread_fork("child_proc", child_proc, entrypoint, (void *)child_tf, 0);


	return 0;
}




int 
sys_getpid(pid_t *pid)
{
	*pid=curproc->pid;
	return 0;
}

int 
sys__exit(int exitcode)
{
	
	if(proc_table[curproc->ppid]->exited==false) 
	{
		curproc->exit_code=_MKWAIT_EXIT(exitcode);
		curproc->exited=true;
		V(curproc->exit_sem);
	}
	else // My parent has exited, I'll just kill myself.
	{
		proc_destroy(curproc);
	}	
	thread_exit();
}


int
sys_waitpid(int pid, userptr_t status, int options, pid_t *retpid)
{

	if (status == NULL)
		return 0;
	if(status == (void *)0x40000000)
		return EFAULT;

	int result;
	struct proc *child_proc;

	if(pid < 1 || pid >= PID_MAX) // PID starts from one
		return ESRCH;

	if (options != 0 && options != WNOHANG) // Can only be one of these two values.
		return EINVAL;


	child_proc = proc_table[pid];

	if(((uintptr_t)status % 4) !=0 )
		return EFAULT; 
	
	if(child_proc == NULL)
		return ESRCH;

	if(child_proc->ppid != curproc->pid)
		return ECHILD; // I am not his parent.

	if(! child_proc->exited) { // If child has not exited already
		if(options == WNOHANG) { // Return 0 if option is WNOHANG
			*retpid = 0;
			return 0;
		}
		else {
			P(child_proc->exit_sem);  // wait for him to exit
		}
	}


	result = copyout(&child_proc->exit_code, status, sizeof(int));

	if(result)
		return result;

	*retpid = pid;

	proc_destroy(child_proc); // Destroy child
	proc_table[pid] = NULL;
	return 0;

}


int sys_execv(char *program, char **args) {

	if (program == NULL)
		return EFAULT;

	if(program == (void *)0x40000000)
		return EFAULT;

	if(program >= (char *)0x80000000)
		return EFAULT;

	if (args==NULL) 
		return EFAULT;

	if(args ==(void *)0x40000000)
		return EFAULT;

	if(args>=(char**)0x80000000)
		return EFAULT;



	int i = 0;

	int result;
	char kern_program[NAME_MAX];
	result = copyinstr((userptr_t)program, kern_program, NAME_MAX, NULL);
	if (result)
		return result;

	if (strcmp(program,"") == 0)
		return EISDIR;

	int argc = 0;
	while(args[argc] != NULL) {
		argc ++;
	}

	// Copy args to kernel args
	char ** kargs = (char **)kmalloc(sizeof(char**));
	if(kargs == NULL)
		return ENOMEM;
	result = copyin((const_userptr_t) args, kargs, sizeof(char **));
	if(result)
		return result;
	size_t size;
	while (args[i] != NULL ) {
		if((args[i] == (void*) 0x40000000) || (args[i] == (char*)0x80000000)){
			kfree(kargs);
			return EFAULT;
		}

		kargs[i] = (char *) kmalloc(sizeof(char) * PATH_MAX);
		if(kargs[i] == NULL) {
			kfree(kargs);
			return ENOMEM;
		}

		result = copyinstr((const_userptr_t) args[i], kargs[i], PATH_MAX, &size);

		i++;
	}
	kargs[i] = NULL;
		


	/* runprogram.c */
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;


	/* Open the file. */
	result = vfs_open(kern_program, O_RDONLY, 0, &v);
	if (result) {
		for(i=0;i<argc;i++)
			kfree(kargs[i]);
		kfree(kargs);
		return result;
	}

	/* We should be a new process. */
	//KASSERT(proc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		for(i=0;i<argc;i++)
			kfree(kargs[i]);
		kfree(kargs);
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		for(i=0;i<argc;i++)
			kfree(kargs[i]);
		kfree(kargs);
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		for(i=0;i<argc;i++)
			kfree(kargs[i]);
		kfree(kargs);
		return result;
	}
	/* end runprogram.c */
	
	// Calculate lengths and offsets for stack pointer
	int args_length[argc];
	int args_padded_length[argc]; // padding + null terminator
	int args_offset[argc+1];
	args_offset[0] = 4*(argc+1); // first arg starts at this offset
	int total_length = 0;

	i = 0;
	while(kargs[i] != NULL) {
		args_length[i] = strlen(kargs[i]);
		if(! ((args_length[i]+1) % 4 == 0)) 
			args_padded_length[i] = ((args_length[i] + 1)+3) / 4 * 4 ;
		else
			args_padded_length[i] = args_length[i]+1;

		args_offset[i+1] = args_offset[i] + args_padded_length[i];
		i++;

	}
	total_length = args_offset[argc]; // clever hack

	// Dispatching kargs to stack pointer
	stackptr -= total_length;
	char* uargs[argc+1]; 

	for( i=0; i<argc; i++) {
		char * dest = (char *)stackptr+args_offset[i];

		result = copyout(kargs[i],(userptr_t)dest,(size_t)args_length[i]+1);
		if (result) {
			for(i=0;i<argc;i++)
				kfree(kargs[i]);
			kfree(kargs);
			return result;
		}
		for (int j = args_length[i]; j < args_padded_length[i] ; j++)
			dest[j] = '\0';
		
		uargs[i] = (char *)dest;
		//kfree(dest);
	}
	uargs[argc] = NULL;

	result = copyout(uargs,(userptr_t)stackptr, sizeof(uargs));
	if (result) {
		for(i=0;i<argc;i++)
			kfree(kargs[i]);
		kfree(kargs);
		return result;
	}

	for(i=0;i<argc;i++)
		kfree(kargs[i]);
	kfree(kargs);
	/* Warp to user mode. */
	enter_new_process(argc, (userptr_t)stackptr /*userspace addr of argv*/,
				NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;

}
