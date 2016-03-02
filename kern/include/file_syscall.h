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

int sys_read(int fd, void* buf, size_t buflen, ssize_t * bytes_read) ;
int sys_write(int fd, void *buf, size_t nbytes, ssize_t * bytes_written);

int sys_dup2(/* Define appropriate arguments */);
int sys_lseek(int fd, off_t pos, int whence, off_t * new_offset);

int sys_chdir(/* Define appropriate arguments */);
int sys__getcwd(void *buf, size_t buflen, size_t * data_length);

#endif

