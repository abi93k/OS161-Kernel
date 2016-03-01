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


int
sys_read(int fd, void* buf, size_t buflen, ssize_t bytes_read) 
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


int
sys_write(int fd, const void *buf, size_t nbytes, ssize_t bytes_written) 
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

int
sys_lseek(int fd, off_t pos, int whence, off_t new_offset) 
{

	struct iovec iov;
	struct uio u;
	off_t offset = 0;
	struct stat f_stat;

	if (fd < 0 || fd > OPEN_MAX) 
		return EBADF; 								// not a valid file descriptor

	struct fdesc * t_fd_entry = curthread->t_fdtable[fd];

	if(t_fd_entry == -1)
		return EBADF; 								// not a valid file descriptor

	if( (whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END) == 0)
		return EINVAL;								// whence is invalid

	lock_acquire(t_fd_entry->lock);
	if(whence == SEEK_SET) {
		offset = pos;
	}
	else if(whence == SEEK_CUR) {
		offset = t_fd_entry->offset + pos;
	}
	else if (whence == SEEK_END) {
		result = VOP_STAT(t_fd_entry->vn, &f_stat);	// get file size to determine end of file
		if(result) {
			lock_release(t_fd_entry->lock);
			return result;
		}
		offset = f_stat.st_size + pos;				// end of file + position
	}
	
	if(offset < 0)
		return EINVAL;								// the resulting seek position would be negative.

	result = VOP_TRYSEEK(t_fd_entry->vn, offset);
	if(result) {
		lock_release(t_fd_entry->lock);
		return result;
	}
	t_fd_entry->offset = offset;
	lock_release(t_fd_entry->lock);

	return offset;
}

int
sys__getcwd(char *buf, size_t buflen, size_t data_length) 
{
	struct iovec iov;
	struct uio u;

	if (buf == NULL)
		return EFAULT; 						//address space pointed to by buf is invalid.


	/* from loadelf.c */
	iov.iov_ubase = buf;					// read data goes into this buffer
	iov.iov_len = buflen;					// length of memory space		 
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;					// amount to read from the file
	u.uio_offset = 0;						// offset 
	u.uio_segflg = UIO_USERISPACE;
	u.uio_rw = UIO_READ;					// READ or WRITE ?
	u.uio_space = curthread->t_addrspace;	// address space of thread

	result = vfs_getcwd(&u);

	if (result) { // error
		return result;
	}

	return 0;

}
