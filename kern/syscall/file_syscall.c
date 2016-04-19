/*
 * Authors: Abhishek Kannan, Arjun Sundaresh
 */
#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <current.h>
#include <file_syscall.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <kern/seek.h>
#include <vnode.h>
#include <uio.h>
#include <synch.h>
#include <vfs.h>
#include <kern/stat.h>
#include <proc.h>


 int
 sys_read(int fd, void* buf, size_t buflen, ssize_t *bytes_read) 
 {
 	struct iovec iov;
 	struct uio u;
 	int result;

 	if (fd < 0 || fd >= OPEN_MAX) 
		return EBADF; 						// not a valid file descriptor

	if(buf==(void*)0x40000000) // Invalid pointer path. Fix this.
		return EFAULT;	

	struct fdesc * p_fd_entry = curproc->p_fdtable[fd];



	if(p_fd_entry == NULL)
		return EBADF; 						// not a valid file descriptor

	if (buf == NULL) 
		return EFAULT; 						//address space pointed to by buf is invalid.


	lock_acquire(p_fd_entry->lock);
	if(((p_fd_entry->flags & O_ACCMODE) != O_RDONLY) && ((p_fd_entry->flags & O_ACCMODE) != O_RDWR ))  {
		lock_release(p_fd_entry->lock);
		return EBADF;
	}
	/* from loadelf.c */
	iov.iov_ubase = buf;					// read data goes into this buffer
	iov.iov_len = buflen;					// length of memory space		 
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;					// amount to read from the file
	u.uio_offset = p_fd_entry->offset;		// offset 
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;					// READ or WRITE ?
	u.uio_space = curproc->p_addrspace;	// address space of thread

	result = VOP_READ(p_fd_entry->vn, &u);

	if (result) { // error
		lock_release(p_fd_entry->lock);
		return result;
	}

	p_fd_entry->offset = u.uio_offset;
	*bytes_read = buflen - u.uio_resid;
	lock_release(p_fd_entry->lock);

	return 0;

}


int
sys_write(int fd, void *buf, size_t buflen, ssize_t *bytes_written) 
{
	struct iovec iov;
	struct uio u;
	int result;

	if (fd < 0 || fd >= OPEN_MAX) 
		return EBADF; 						// not a valid file descriptor

	if(buf==(void*)0x40000000) // Invalid pointer path. Fix this.
		return EFAULT;	
	

	struct fdesc * p_fd_entry = curproc->p_fdtable[fd];

	if(p_fd_entry == NULL)
		return EBADF; 						// not a valid file descriptor

	if (buf == NULL)
		return EFAULT; 						//address space pointed to by buf is invalid.


	lock_acquire(p_fd_entry->lock);
	if((p_fd_entry->flags & O_ACCMODE) != O_WRONLY && (p_fd_entry->flags & O_ACCMODE) != O_RDWR )  {
		lock_release(p_fd_entry->lock);
		return EBADF;
	}
	/* from loadelf.c */
	iov.iov_ubase = buf;					// data to be written to the file
	iov.iov_len = buflen;					// length of memory space		 
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;					// amount to read from the file
	u.uio_offset = p_fd_entry->offset;		// offset 
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;					// READ or WRITE ?
	u.uio_space = curproc->p_addrspace;	// address space of thread

	result = VOP_WRITE(p_fd_entry->vn, &u);

	if (result) { // error
		lock_release(p_fd_entry->lock);
		return result;
	}

	p_fd_entry->offset = u.uio_offset;
	*bytes_written = buflen - u.uio_resid;
	lock_release(p_fd_entry->lock);

	return 0;
}

int
sys_lseek(int fd, off_t pos, int whence, off_t *new_offset) 
{


	off_t offset = 0;
	struct stat f_stat;
	int result;

	if (fd < 0 || fd >= OPEN_MAX) 
		return EBADF; 								// not a valid file descriptor



	struct fdesc * p_fd_entry = curproc->p_fdtable[fd];

	if(p_fd_entry == NULL)
		return EBADF; 								// not a valid file descriptor

	if( (whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END) == 0)
		return EINVAL;								// whence is invalid

	lock_acquire(p_fd_entry->lock);
	if(whence == SEEK_SET) {
		offset = pos;
	}
	else if(whence == SEEK_CUR) {
		offset = p_fd_entry->offset + pos;
	}
	else if (whence == SEEK_END) {
		result = VOP_STAT(p_fd_entry->vn, &f_stat);	// get file size to determine end of file
		if(result) {
			lock_release(p_fd_entry->lock);
			return result;
		}
		offset = f_stat.st_size + pos;				// end of file + position
	}
	
	if(offset < 0) {
		lock_release(p_fd_entry->lock);
		return EINVAL;								// the resulting seek position would be negative.
	}

	result = VOP_ISSEEKABLE(p_fd_entry->vn);
	if(!result) {
		lock_release(p_fd_entry->lock);
		return ESPIPE;
	}
	p_fd_entry->offset = offset;
	*new_offset = offset;
	lock_release(p_fd_entry->lock);

	return 0;
}

int
sys__getcwd(void *buf, size_t buflen, size_t *data_length) 
{
	struct iovec iov;
	struct uio u;
	int result;

	if (buf == NULL)
		return EFAULT; 						//address space pointed to by buf is invalid.


	/* from loadelf.c */
	iov.iov_ubase = buf;					// read data goes into this buffer
	iov.iov_len = buflen;					// length of memory space		 
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;					// amount to read from the file
	u.uio_offset = 0;						// offset 
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;					// READ or WRITE ?
	u.uio_space = curproc->p_addrspace;	// address space of thread

	result = vfs_getcwd(&u);

	if (result) { // error
		return result;
	}
	*data_length = buflen - u.uio_resid;
	return 0;
}

int sys_open(char *filepath, int flags, mode_t mode, int *retval)
{

	size_t got;
	struct stat f_stat;
	int result;
	int fd=3;
	if(filepath==NULL)
		return EFAULT;

	if(filepath==(void*)0x40000000) // Invalid pointer path. Fix this.
		return EFAULT;

	if(((flags & O_RDONLY) !=O_RDONLY) && ((flags & O_WRONLY) !=O_WRONLY) && ((flags & O_RDWR) !=O_RDWR))
		return EINVAL;

	char *kernel_buffer = kmalloc(sizeof(char)*PATH_MAX);	

	result=copyinstr((const_userptr_t) filepath, kernel_buffer, PATH_MAX, &got);
	if(result) {
		kfree(kernel_buffer);
		return result;
	}


	struct fdesc *p_fd_entry=kmalloc(sizeof(struct fdesc)); 

	result = vfs_open(kernel_buffer,flags,mode,&p_fd_entry->vn);
	kfree(kernel_buffer);
	if(result) {
		kfree(p_fd_entry);
		return result;
	}

	if((flags & O_APPEND)==O_APPEND)
	{
		result = VOP_STAT(p_fd_entry->vn,&f_stat);
		if(result) {
			kfree(p_fd_entry);
			return result;
		}
		p_fd_entry->offset=f_stat.st_size;
	}
	else
		p_fd_entry->offset=0;

  	//TODO add name
	p_fd_entry->reference_count = 1;
	p_fd_entry->flags = flags;
	p_fd_entry->lock = lock_create(kernel_buffer);


	while(curproc->p_fdtable[fd]!=NULL && fd < OPEN_MAX)
	{
		fd++;
	}

	if(fd == OPEN_MAX) {
		kfree(p_fd_entry);
		return ENFILE;  
	}

	curproc->p_fdtable[fd] = p_fd_entry;

	*retval = fd;

	return 0;
}

int sys_close(int fd)
{


	if(fd < 0 || fd >= OPEN_MAX)
		return EBADF;

	struct fdesc* p_fd_entry = curproc->p_fdtable[fd]; 

	if(p_fd_entry == NULL)
		return EBADF;


	curproc->p_fdtable[fd]=NULL; 

	p_fd_entry->reference_count -= 1;

	if(p_fd_entry->reference_count==0)
	{
		lock_destroy(p_fd_entry->lock);
		vfs_close(p_fd_entry->vn);
		kfree(p_fd_entry); 
	}

	return 0;
}

int sys_chdir(char *newpath)
{

	int result;
	char *kernel_buffer = kmalloc(sizeof(char)*PATH_MAX);	
	size_t actual;


	result = copyinstr((const_userptr_t) newpath, kernel_buffer, PATH_MAX, &actual);
	if(result) {
		kfree(kernel_buffer);
		return result;
	}

	result = vfs_chdir(kernel_buffer);
	kfree(kernel_buffer);

	if(result)
		return EFAULT;

	return 0;
}



int sys_dup2(int oldfd, int newfd, int *retval)
{

	if(oldfd<0 || newfd<0 || oldfd >= OPEN_MAX || newfd >= OPEN_MAX )
		return EBADF;

	if(curproc->p_fdtable[oldfd] == NULL)
		return EBADF;

	if(oldfd==newfd) {
		*retval=newfd;
		return 0;

	}

	if(curproc->p_fdtable[newfd]!=NULL)
		sys_close(newfd);


	lock_acquire(curproc->p_fdtable[oldfd]->lock);
	curproc->p_fdtable[oldfd]->reference_count+=1;

	curproc->p_fdtable[newfd]=curproc->p_fdtable[oldfd];

	lock_release(curproc->p_fdtable[oldfd]->lock);

	*retval=newfd;

	return 0;

}
