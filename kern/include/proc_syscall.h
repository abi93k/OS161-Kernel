#ifndef _PROC_SYSCALL_H_
#define _PROC_SYSCALL_H_


#include <types.h>
#include <limits.h>



/* Add file system call functions here */

void entrypoint(void* tf, unsigned long data);
int sys_fork(struct trapframe* tf, int *retval);
int sys_exit(int fd);

#endif

