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


	/* TODO Copy file table properly here */
	
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
	else
	{
		proc_destroy(curproc);
	}	
	thread_exit();
}


int
sys_waitpid(int pid, userptr_t status, int options, pid_t *retpid)
{
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
