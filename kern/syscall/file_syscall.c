#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <copyinout.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <vnode.h>
#include <synch.h>
#include <kern/stat.h>
#include <file_syscall.h>
#include <thread.h>


/*
 * TODO
 * Why hardcoded open flags ? Change to variables     ||DONE||
 * Add synchronization at appropriate places          ||DONE||
 * prefix sys_ to all functions                       ||DONE||
 *
 * |||||||||||||||||||||||||||||||| NOTE |||||||||||||||||||||||
 * Check all the areas you put a flag , search for " ||CHECK IF OKAY|| " to see those places.
 * I have added the retval values for open() and dup2()
 */

 int sys_open(char *filepath, int flags, mode_t mode, int *retval)
 {

  
  struct lock* locks=lock_create("pushlock");

  //INITIALIZE fdesc OBJECT
  struct fdesc *fd=kmalloc(sizeof(struct fdesc)); 

  //Step 1 -ERROR - Check if filepath is a invalid pointer.
  if(filepath==NULL)
    return EFAULT; 

  //Step 2 - ERROR - Check if the flags are a valid.
  if((flags & O_RDONLY) !=O_RDONLY || (flags & O_WRONLY) !=O_WRONLY || (flags & O_RDWR) !=O_RDWR)
    return EINVAL;
  if(flags < 0 || flags >OPEN_MAX)
    return EINVAL;

  //Step 3 - Use copyinstr() to copy the filepath to kernel memory.
  int copyresult;
  char *dest =kmalloc(sizeof(char));
  size_t len=sizeof(dest);
  size_t actual;

  copyresult=copyinstr((const_userptr_t) filepath, dest, len, &actual);
  if(copyresult)
    return copyresult;
  //Step 5 - Use vfs_open() to open the file.
  int result;

  result = vfs_open(filepath,flags,mode,&fd->vn);
  if(result)
    return result;

  //Step 6 - Check for Append and set offset value accordingly.
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

  //Step 7 - Create a fdesc object and keep data into its data items.

  //Figure out thow to add name
  //fd->name='c';
  fd->reference_count=1;
  fd->flags=flags;
  fd->lock= lock_create(dest);

  //Step 8 - Find the next open node in curproc->t_fdtble.
  int index=3;

  lock_acquire(locks);
  while(curproc->t_fdtable[index]!=0 && index<PATH_MAX)
  {
    index++;
  }


  // Check for ENFILE
  if(index > OPEN_MAX)
    return ENFILE;  

  curproc->t_fdtable[index]= kmalloc(sizeof(struct fdesc*));

  //Step 10 - Copy the fdesc object into the next empty crthread->t_fdtable index.
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

  //Step 1 -  Check if the fd is valid
  if(fd<0 || fd>PATH_MAX)
    return EBADF;


  //Step 2 - Check if there is a entry in the fdtable for that fd
  if(filedesc==NULL)
    return EBADF;

  lock_acquire(locks);

  //Step 3 - Decrement the reference count and free memory if zero.
  filedesc->reference_count -= 1;
  if(filedesc->reference_count==0)
  {

    //|| CHECK IF OKAY ||
    // What is tempfd ? It is never declared. 
    // You removed tempfd and then try to access it ? This will cause segmentation fault.
    kfree(filedesc); 
    filedesc=NULL; 
    lock_destroy(filedesc->lock);
  }
    lock_release(locks);
  //Step 4 - Use vfs_close()
 
  vfs_close(filedesc->vn);
  
  lock_destroy(locks);
  
  return 0;
}

int sys_chdir(const char *newpath)
{

  //Step 3 - Use copyinstr() to copy the filepath to kernel memory.
  int copyresult;
  char *dest =kmalloc(sizeof(char));
  

  // || CHECK IF OKAY ||
  // You are using len on an empty char array ?
  size_t len=PATH_MAX; 
  size_t actual;

  //|| CHECK IF OKAY ||
  // What is filepath ?
  copyresult=copyinstr((const_userptr_t) newpath, dest, len, &actual);
  if(copyresult)
    return copyresult;
  //Step 4
  int result;
  result=vfs_chdir(dest);

  if(result<0)
    return EFAULT;

  return 0;
}



int sys_dup2(int oldfd,int newfd, int *retval)
{


  //Step 1 -  Check if newfd and old are valid          
  if(oldfd<0 || newfd<0 || curproc->t_fdtable[oldfd]==NULL)
    return EBADF;

  //Step 3 - Check if both newfd and oldfd are the same.  
  if(oldfd==newfd)
    return newfd;

  //Step 4 - Close the newfd using sys_close
  if(curproc->t_fdtable[newfd]!=NULL)
  {
    
    sys_close(newfd);
    
  }
  else
    curproc->t_fdtable[newfd]= kmalloc(sizeof(struct fdesc*));


  //Step 5
  lock_acquire(curproc->t_fdtable[oldfd]->lock);
  //Step 6
  curproc->t_fdtable[oldfd]->reference_count+=1;

  //Step 7
  curproc->t_fdtable[newfd]=curproc->t_fdtable[oldfd];

  //Step 8
  lock_release(curproc->t_fdtable[oldfd]->lock);
  *retval=newfd;
  return 0;


}
