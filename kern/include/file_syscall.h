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

int sys_open(/* Define appropriate arguments */);
int sys_close(/* Define appropriate arguments */);

ssize_t sys_read(int fd, void* buf, size_t buflen) ;
ssize_t sys_write(int fd, const void *buf, size_t nbytes);

int sys_dup2(/* Define appropriate arguments */);
off_t lseek(int fd, off_t pos, int whence);

int sys_chdir(/* Define appropriate arguments */);
int __getcwd(char *buf, size_t buflen);

#endif

