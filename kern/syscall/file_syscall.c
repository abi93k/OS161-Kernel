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

	if (fd < 0 || fd > OPEN_MAX) 
		return EBADF; 						// not a valid file descriptor

	struct fdesc * t_fd_entry = curproc->t_fdtable[fd];

	if(t_fd_entry == NULL)
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
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;					// READ or WRITE ?
	u.uio_space = curproc->p_addrspace;	// address space of thread

	result = VOP_READ(t_fd_entry->vn, &u);

	if (result) { // error
		lock_release(t_fd_entry->lock);
		return result;
	}

	t_fd_entry->offset = u.uio_offset;
	*bytes_read = buflen - u.uio_resid;
	lock_release(t_fd_entry->lock);

	return 0;

}


int
sys_write(int fd, void *buf, size_t buflen, ssize_t *bytes_written) 
{
	struct iovec iov;
	struct uio u;
	int result;

	if (fd < 0 || fd > OPEN_MAX) 
		return EBADF; 						// not a valid file descriptor

	struct fdesc * t_fd_entry = curproc->t_fdtable[fd];

	if(t_fd_entry == NULL)
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
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;					// READ or WRITE ?
	u.uio_space = curproc->p_addrspace;	// address space of thread

	result = VOP_WRITE(t_fd_entry->vn, &u);

	if (result) { // error
		lock_release(t_fd_entry->lock);
		return result;
	}

	t_fd_entry->offset = u.uio_offset;
  *bytes_written = buflen - u.uio_resid;
	lock_release(t_fd_entry->lock);

	return 0;
}

int
sys_lseek(int fd, off_t pos, int whence, off_t *new_offset) 
{


	off_t offset = 0;
	struct stat f_stat;
	int result;

	if (fd < 0 || fd > OPEN_MAX) 
		return EBADF; 								// not a valid file descriptor

	struct fdesc * t_fd_entry = curproc->t_fdtable[fd];

	if(t_fd_entry == NULL)
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

	result = VOP_ISSEEKABLE(t_fd_entry->vn);
	if(result) {
		lock_release(t_fd_entry->lock);
		return result;
	}
	t_fd_entry->offset = offset;
  	*new_offset = offset;
	lock_release(t_fd_entry->lock);

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

 int sys_open(char *filepath, int flags, mode_t mode, int *retval)
 {

  
  struct lock* locks=lock_create("pushlock");

  struct fdesc *fd=kmalloc(sizeof(struct fdesc)); 

  if(filepath==NULL)
    return EFAULT; 

  if(((flags & O_RDONLY) !=O_RDONLY) && ((flags & O_WRONLY) !=O_WRONLY) && ((flags & O_RDWR) !=O_RDWR))
    return EINVAL;
  if(flags < 0 || flags >OPEN_MAX)
    return EINVAL;

  int copyresult;
  char *dest =kmalloc(sizeof(char)*PATH_MAX);
  size_t actual;

  copyresult=copyinstr((const_userptr_t) filepath, dest, PATH_MAX, &actual);
  if(copyresult)
    return copyresult;
  int result;

  result = vfs_open(filepath,flags,mode,&fd->vn);
  if(result)
    return result;

  if((flags & O_APPEND)==O_APPEND)
    {
      struct  stat ft;
      int res=VOP_STAT(fd->vn,&ft);
      if(res)
        return res;
      fd->offset=ft.st_size;
    }
  else
    fd->offset=0;


  //TODO add name
  fd->reference_count=1;
  fd->flags=flags;
  fd->lock= lock_create(dest);

  int index=3;

  lock_acquire(locks);
  while(curproc->t_fdtable[index]!=0 && index<PATH_MAX)
  {
    index++;
  }


  if(index > OPEN_MAX)
    return ENFILE;  

  curproc->t_fdtable[index]= kmalloc(sizeof(struct fdesc*));

  curproc->t_fdtable[index]=fd;

  lock_release(locks);
  lock_destroy(locks);
  *retval= index;
  return 0;
}

int sys_close(int fd)
{

  struct lock* locks=lock_create("closelock");

  struct fdesc* filedesc = curproc->t_fdtable[fd]; 

  if(fd<0 || fd>PATH_MAX)
    return EBADF;

  if(filedesc==NULL)
    return EBADF;

  lock_acquire(locks);

  filedesc->reference_count -= 1;
  if(filedesc->reference_count==0)
  {

    lock_destroy(filedesc->lock);
    vfs_close(filedesc->vn);
    kfree(filedesc); 
    filedesc=NULL; 

    
  }
    lock_release(locks);  
  
  lock_destroy(locks);
  
  return 0;
}

int sys_chdir(const char *newpath)
{

  int copyresult;
  char *dest =kmalloc(sizeof(char));
  


  size_t len=PATH_MAX; 
  size_t actual;


  copyresult=copyinstr((const_userptr_t) newpath, dest, len, &actual);
  if(copyresult)
    return copyresult;
  int result;
  result=vfs_chdir(dest);

  if(result<0)
    return EFAULT;

  return 0;
}



int sys_dup2(int oldfd,int newfd, int *retval)
{


  if(oldfd<0 || newfd<0 || curproc->t_fdtable[oldfd]==NULL)
    return EBADF;

  if(oldfd==newfd)
    return newfd;

  if(curproc->t_fdtable[newfd]!=NULL)
  {
    
    sys_close(newfd);
    
  }
  else
    curproc->t_fdtable[newfd]= kmalloc(sizeof(struct fdesc*));


  lock_acquire(curproc->t_fdtable[oldfd]->lock);
  curproc->t_fdtable[oldfd]->reference_count+=1;

  curproc->t_fdtable[newfd]=curproc->t_fdtable[oldfd];

  lock_release(curproc->t_fdtable[oldfd]->lock);
  *retval=newfd;
  return 0;


}
