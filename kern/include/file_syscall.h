#ifndef _FILE_SYSCALL_H_
#define _FILE_SYSCALL_H_


#include <types.h>
#include <limits.h>

struct fdesc
{
	char name[NAME_MAX]; /* NAME_MAX is define in limits.h */
	struct vnode *vn;

	int flags;
	off_t offset;
	int reference_count;
	struct lock *lock;

};

/* Add file system call functions here */

int sys_open(/* Define appropriate arguments */);
int sys_close(/* Define appropriate arguments */);

int sys_read(/* Define appropriate arguments */);
int sys_write(/* Define appropriate arguments */);

int sys_dup2(/* Define appropriate arguments */);
int sys_lseek(/* Define appropriate arguments */);

int sys_chdir(/* Define appropriate arguments */);
int sys___getcwd(/* Define appropriate arguments */);


#endif

