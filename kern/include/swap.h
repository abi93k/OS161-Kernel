#ifndef _SWAP_H_
#define _SWAP_H_

#include <bitmap.h>

struct vnode *disk;
struct bitmap *disk_map;
struct lock *disk_map_lock;
bool swap_enabled;
struct semaphore *tlb_sem;
struct lock * coremap_lock1;



int choose_victim(void);
int swapout(int coremap_index);
int swapin(paddr_t paddr,short position);
int disk_write(vaddr_t vaddr, short position);
int disk_read(vaddr_t vaddr, short position);
void swapspace_bootstrap(void);
unsigned allocate_disk_index(void);
void deallocate_disk_index(unsigned index);


#endif /* _SWAP_H_ */
