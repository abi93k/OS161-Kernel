#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <string.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <string.h>
#include <synch.h>
#include <file_syscall.h>
#include <thread.h>
#include <unistd.h>

/*
 * TODO
 * Why hardcoded open flags ? Change to variables 
 * Add synchronization at appropriate places
 * prefix sys_ to all functions
 *
 * SIDE NOTE
 * Give clear commit messages describing what was done in one line. It will be easier to roll back 
 * if we mess up the source files during merge. You don't have to mention your name, it gets recorded
 * anyway
 */
 int open(char *filepath, int flags, mode_t mode)
 {

  //INITIALIZE fdesc OBJECT
  struct fdesc *fd=kmalloc(sizeof(fdesc)); 

  //Step 1 -ERROR - Check if filepath is a invalid pointer.
  if(filepath==NULL)
    return EFAULT; 

  //Step 2 - ERROR - Check if the flags are a valid.
  if(flags && 0 !=0 || flags && 1 !=1 || flags && 2 !=2)
    return EINVAL;
  if(flag < 0 || flag >OPEN_MAX)
    return EINVAL;

  //Step 3 - Use copyinstr() to copy the filepath to kernel memory.
  int copyresult;
  char *dest;
  size_t len=sizeof(dest);
  size_t actual;

  copyresult=copyinstr((const_userptr_t) filepath, dest, len, &actual);

  //Step 5 - Use vfs_open() to open the file.
  int result;

  result = vfs_open(filepath,flags,mode,&fd->vn);
  if(result < 0)
    return result;

  //Step 6 - Check for Append and set offset value accordingly.
  if(flag && 32==32)
    fd->offset=vop_stat(fd->vn);
  else
    fd->offset=0;

  //Step 7 - Create a fdesc object and keep data into its data items.
  strcpy(fd->name,dest);
  fd->reference_count=1;
  fd->flags=flags;
  fd->lock= lock_create(dest);

  //Step 8 - Find the next open node in curthread->t_fdtble.
  int index=3;
  while(curthread->t_fdtable[index]!=0 && index<PATH_MAX)
  {
    index++
  }


  // Check for ENFILE
  if(index > OPEN_MAX)
    return ENFILE;  

  curthread->t_fdtable[index]= kmalloc(sizeof(strict fdesc*));

  //Step 10 - Copy the fdesc object into the next empty crthread->t_fdtable index.
  curthread->t_fdtable[index]=fd;

  return index;
}

int close(int fd)
{

  filedesc = curthread->t_fdtable[fd]; 

  //Step 1 -  Check if the fd is valid
  if(fd<0 || fd>PATH_MAX)
    return EBADF;


  //Step 2 - Check if there is a entry in the fdtable for that fd
  if(filedesc==NULL)
    return EBADF;

  //Step 3 - Decrement the reference count and free memory if zero.
  filedesc->reference_count -= 1;
  if(filedesc->reference_count==0)
  {
    // FLAG
    // What is tempfd ? It is never declared. 
    // You removed tempfd and then try to access it ? This will cause segmentation fault.
    kfree(tempfd); 
    tempfd=NULL; 
    lock_destroy(tempfd->lock);
  }

  //Step 4 - Use vfs_close()
  int result;
  result=vfs_close(filedesc->vn);

  return 0;
}

int chdir(const char *newpath)
{

  //Step 3 - Use copyinstr() to copy the filepath to kernel memory.
  int copyresult;
  char *dest;
  // FLAG 
  // You are using len on an empty char array ?
  size_t len=sizeof(dest); 
  size_t actual;

  // FLAG
  // What is filepath ?
  copyresult=copyinstr((const_userptr_t) filepath, dest, len, &actual);

  //Step 4
  int result;
  result=vfs_chdir(dest);

  if(result<0)
    return EFAULT;

  return 0;
}



int dup2(int oldfd,int newfd)
{


  //Step 1 -  Check if newfd and old are valid          
  if(oldfd<0 || newfd<0 || curthread->t_fdtable[oldfd]==NULL)
    return EBADAF;

  //Step 3 - Check if both newfd and oldfd are the same.  
  if(oldfd==newfd)
    return newfd;

  //Step 4 - Close the newfd using sys_close
  if(curthread->t_fdtable[newfd]!=NULL)
  {
    int result;
    result=sys_close(newfd);
  }
  else
    curthread->t_fdtable[newfd]= kmalloc(sizeof(strict fdesc*));


  //Step 5
  lock_aquire(curthread->t_fdtable[oldfd]->lock);
  //Step 6
  curthread->t_fdtable[oldfd]->reference_count+=1;

  //Step 7
  curthread->t_fdtable[newfd]=curthread->t_fdtable[oldfd];

  //Step 8
  lock_release(curthread->t_fdtable[oldfd]);
  return newfd;


}