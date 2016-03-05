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


void entrypoint(void* data1,unsigned long data2)
{
	struct trapframe *tf1,tf2;
	tf1 = (struct trapframe*)data1;
	tf1->tf_v0=0;
	tf1->tf_a3=0;
	tf1->tf_epc+=4;
	//int res_addthread=addthread(curproc,curthread);
	//if(res_addthread)
		//return res_addthread
	data2++;
	tf2=*tf1;
	mips_usermode(&tf2);

}
int sys_fork(struct trapframe* tf, int *retval)
{


	// Step 1 - Createa new proc object and call create_proc
	struct proc *procs;

	procs = proc_create_runprogram("fork_proc");
	if(procs==NULL)
		return ENOMEM;

	// Step 3 - Copy the addrspace ino the proc object->addrspace using as_copy
	int res_addrcopy;
	res_addrcopy=as_copy(curproc->p_addrspace, &procs->p_addrspace);

	if(res_addrcopy)
	{
		return res_addrcopy;
	}

	
	// Step 2,4 - copy the trapframe to the child
	struct trapframe *new_tf = kmalloc(sizeof(struct trapframe));
	*new_tf=*tf;


	//Step 5 - Find spot in proc_table and fill it with the child process
	int i=3;
	while(procs->p_fdtable[i]!=NULL && i<=OPEN_MAX)
	{
	procs->p_fdtable[i]=curproc->p_fdtable[i];
	i++;
	}
	
	procs->ppid=curproc->pid;

	proc_table[procs->pid]=procs;

	//  Step 6 - Call thread_fork
	int res_fork;
	unsigned long dummy=0;
	res_fork=thread_fork("TheChild", procs,entrypoint,(void *)new_tf,dummy);
	if(res_fork)
	{
		return res_fork;
	}

	*retval=0;
	return 0;
}
