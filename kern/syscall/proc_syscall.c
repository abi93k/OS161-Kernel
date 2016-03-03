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
#include <kern/stat.h>
#include <file_syscall.h>
#include <thread.h>




/*
*    					||TODO||
*
* Step 1 - Createa new proc object and call create_proc
* Step 2 -  Create a new trpframe object and copy it to kernel memory
* Step 3 - Copy the addrspace ino the proc object->addrspace using as_copy
* Step 4 -  Copy the parent trapframe to the new trapframe object
* Step 5 - Find spot in proc_table and fill it with the child process
* Step 6 -  Use thread_fork to create a child.
* 
*/


void entrypoint(struct trapframe *tf)
{
	tf->v0=0;
	int res_addthread=addthread(curproc,curthread);
	if(res_addthread)
		return res_addthread

	mips_usermode(tf);

}
pid_t sys_fork(struct trapframe* tf int *retval)
{
	// Step 1 - Createa new proc object and call create_proc
	struct proc *proc;
	proc=proc_create("forkproc");
	if(proc==NULL)
		return ENOMEM;

	// Step 3 - Copy the addrspace ino the proc object->addrspace using as_copy
	int res_addrcopy;
	res_addrcopy=as_copy(curproc->p_addrspace, proc->p_addrspace);

	if(res_addrcopy)
	{
		return res_addrcopy;
	}

	
	// Step 2,4 - copy the trapframe to the child
	struct trapframe *new_tf = kmalloc(sizeof(struct trapframe));
	newtf=tf;


	//Step 5 - Find spot in proc_table and fill it with the child process
	while(proc_table[i]!=NULL && i<PID_MAX)
		i++	
	if(i==PID_MAX)
		return ENPROC;

	proc_table[i]=proc;
	proc->pid=i;
	proc->t_fdtable=curproc->t_fdtable;
	proc->ppid=curproc->pid;

	//  Step 6 - Call thread_fork
	int res_fork;
	res_fork=thread_fork("TheChild", proc,entrypoint,(struct trapframe*)tf,NULL);
	if(res_fork)
	{
		return res_fork;
	}

	return 0;
}