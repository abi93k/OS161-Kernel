#ifndef _FILE_SYSCALL_H_
#define _FILE_SYSCALL_H_


#include <types.h>
#include <limits.h>

struct fdesc
{
	char name[NAME_MAX]; /* NAME_MAX is defined in limits.h */
	struct vnode *vn;

	int flags;
	off_t offset;
	int reference_count;
	struct lock *lock;

};

/* Add file system call functions here */

int sys_open(char *filepath, int flags, mode_t mode, int *retval);
int sys_close(int fd);

int sys_read(/* Define appropriate arguments */);
int sys_write(/* Define appropriate arguments */);

int sys_dup2(int oldfd,int newfd, int *retval);
int sys_lseek(/* Define appropriate arguments */);

int sys_chdir(const char *newpath);
int sys___getcwd(/* Define appropriate arguments */);


#endif

