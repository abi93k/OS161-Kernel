/*
 * Authors: Abhishek Kannan, Arjun Sundaresh
 */
#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <current.h>
#include <file_syscall.h>
#include <kern/errno.h>
#include <vnode.h>
#include <uio.h>


ssize_t
sys_read(int fd, void* buf, size_t buflen) 
{
	struct iovec iov;
	struct uio u;

	if (fd < 0 || fd > OPEN_MAX) 
		return EBADF; 						// not a valid file descriptor

	struct fdesc * t_fd_entry = curthread->t_fdtable[fd];

	if(t_fd_entry == -1)
		return EBADF; 						// not a valid file descriptor

	if (buf == NULL)
		return EFAULT; 						//address space pointed to by buf is invalid.


	lock_acquire(t_fd_entry->lock);

	/* from loadelf.c */
	iov.iov_ubase = buf;					// read data goes into this buffer
	iov.iov_len = buflen;					// length of memory space		 
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;					// amount to read from the file
	u.uio_offset = t_fd_entry->offset;		// offset 
	u.uio_segflg = UIO_USERISPACE;
	u.uio_rw = UIO_READ;					// READ or WRITE ?
	u.uio_space = curthread->t_addrspace;	// address space of thread

	result = VOP_READ(t_fd_entry->vn, &u);

	if (result) { // error
		lock_release(t_fd_entry->lock);
		return result;
	}

	t_fd_entry->offset = u.uio_offset;
	lock_release(t_fd_entry->lock);

	return 0;

}


ssize_t
sys_write(int fd, const void *buf, size_t nbytes) {
	struct iovec iov;
	struct uio u;

	if (fd < 0 || fd > OPEN_MAX) 
		return EBADF; 						// not a valid file descriptor

	struct fdesc * t_fd_entry = curthread->t_fdtable[fd];

	if(t_fd_entry == -1)
		return EBADF; 						// not a valid file descriptor

	if (buf == NULL)
		return EFAULT; 						//address space pointed to by buf is invalid.


	lock_acquire(t_fd_entry->lock);

	/* from loadelf.c */
	iov.iov_ubase = buf;					// data to be written to the file
	iov.iov_len = buflen;					// length of memory space		 
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;					// amount to read from the file
	u.uio_offset = t_fd_entry->offset;		// offset 
	u.uio_segflg = UIO_USERISPACE;
	u.uio_rw = UIO_WRITE;					// READ or WRITE ?
	u.uio_space = curthread->t_addrspace;	// address space of thread

	result = VOP_WRITE(t_fd_entry->vn, &u);

	if (result) { // error
		lock_release(t_fd_entry->lock);
		return result;
	}

	t_fd_entry->offset = u.uio_offset;
	lock_release(t_fd_entry->lock);

	return 0;
}

off_t
lseek(int fd, off_t pos, int whence) {
	/* TODO */
}

int
__getcwd(char *buf, size_t buflen) {
	/* TODO */
}
