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

int sys_read(int fd, void* buf, size_t buflen, ssize_t * bytes_read) ;
int sys_write(int fd, void *buf, size_t nbytes, ssize_t * bytes_written);

int sys_dup2(int oldfd,int newfd, int *retval);
int sys_lseek(int fd, off_t pos, int whence, off_t * new_offset);

int sys_chdir(char *newpath);
int sys__getcwd(void *buf, size_t buflen, size_t * data_length);


#endif

