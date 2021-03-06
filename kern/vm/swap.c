#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <synch.h>
#include <vm.h>
#include <pagetable.h>
#include <addrspace.h>
#include <swap.h>
#include <uio.h>
#include <vnode.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <stat.h>
#include <cpu.h>
#include <clock.h>

int choose_victim(/*Add as necessary*/) {
    //time_t seconds = MAX_TIME;
    //uint32_t nanoseconds = 0;
    if(swap_enabled == false)
        panic("choose_victim: swapping not enabled");

    /* LRU */

    while(1) {
        if(coremap[clock_hand].page!=NULL) {

            if (coremap[clock_hand].state==FIXED || coremap[clock_hand].state==VICTIM || coremap[clock_hand].pinned==1){
                clock_hand = (clock_hand+1)%no_of_coremap_entries;
                continue;
            }    

            if(coremap[clock_hand].accessed == 1 ) {
                coremap[clock_hand].accessed = 0;
                clock_hand = (clock_hand+1)%no_of_coremap_entries;
            }
            else {
                KASSERT(coremap[clock_hand].state!=FIXED);
                KASSERT(coremap[clock_hand].state!=VICTIM);
                return clock_hand;
            }
        }

        clock_hand = (clock_hand+1)%no_of_coremap_entries;

    }
    /*
    int victim = -1;

    for (int i =0; i<no_of_coremap_entries;i++) {
        if(coremap[i].state == FREE) {
            return i;
        }
        if (coremap[i].state==FIXED || coremap[i].state==VICTIM || coremap[i].pinned==1){
            
            continue;
        } 
        else {
            if(coremap[i].seconds<seconds) {
                seconds = coremap[i].seconds;
                nanoseconds = coremap[i].nanoseconds;
                victim = i;
                  
            }
            else if(coremap[i].seconds==seconds) {
                if(coremap[i].nanoseconds<nanoseconds) {
                    seconds = coremap[i].seconds;
                    nanoseconds = coremap[i].nanoseconds;
                    victim = i;
                }
            }

        }
    }
    KASSERT(coremap[victim].state!=FIXED);
    KASSERT(coremap[victim].state!=VICTIM);
    return victim;
    */
}

int swapout(int coremap_index) {

    if(swap_enabled == false)
        panic("swapout: swapping not enabled");    

	vaddr_t vaddr = coremap[coremap_index].vaddr;
    unsigned cpu = coremap[coremap_index].cpu;

	//vm_tlbflush(vaddr & PAGE_FRAME);
    ipi_tlbshootdown_allcpus(&(const struct tlbshootdown){vaddr, tlb_sem, cpu});

	KASSERT(coremap[coremap_index].as!=NULL);
	
    //struct pte* target = pte_get(coremap[coremap_index].as,vaddr);
    struct pte * target =  coremap[coremap_index].page; // O(1)
	short position = target -> location_on_disk;

	paddr_t paddr = CM_TO_PADDR(coremap_index);		

	int result = disk_write(PADDR_TO_KVADDR(paddr),position);
    target->in_memory = 0;
	target->paddr = 1; // Lousy hack.


	(void)result;

	return 0;	
}

int swapin(paddr_t paddr,short position) {
    if(swap_enabled == false)
        panic("swapin: swapping not enabled");    

	return disk_read(PADDR_TO_KVADDR(paddr),position);

}



int disk_write(vaddr_t vaddr, short position) {
    if(swap_enabled == false)
        panic("disk_write: swapping not enabled");

	struct iovec iov;
    struct uio u;
    uio_kinit(&iov, &u, (void*)vaddr, PAGE_SIZE, PAGE_SIZE * position, UIO_WRITE);
    return VOP_WRITE(disk,&u);


}
int disk_read(vaddr_t vaddr, short position) {
    if(swap_enabled == false)
        panic("disk_read: swapping not enabled");

	struct iovec iov;
    struct uio u;
    uio_kinit(&iov, &u, (void*)vaddr, PAGE_SIZE, PAGE_SIZE * position, UIO_READ);
    return VOP_READ(disk,&u);


}


void swapspace_bootstrap() {
    swap_enabled = false;
    struct stat f_stat;
	char *path = kstrdup("lhd0raw:");
    //fault_lock = lock_create("fault lock");

	int err = vfs_open(path, O_RDWR, 0, &disk);
	if(!err) { 
        VOP_STAT(disk, &f_stat);
        disk_map = bitmap_create(f_stat.st_size / PAGE_SIZE);
        kprintf("\n Number of swap slots %d \n",(int)(f_stat.st_size / PAGE_SIZE));        
        disk_map_lock = lock_create("disk map lock");
        allocate_disk_index();
        swap_enabled = true;
        tlb_sem = sem_create("Shootdown", 0);
    }




}

unsigned allocate_disk_index() {
    if(swap_enabled == false)
        return 0;
    unsigned index;

    lock_acquire(disk_map_lock);
    if (bitmap_alloc(disk_map, &index))
        panic("no space on disk");

    lock_release(disk_map_lock);
    return index;
}

void deallocate_disk_index(unsigned index) {
    if(swap_enabled == false)
        return;

    lock_acquire(disk_map_lock);

    if(bitmap_isset(disk_map, index) == false) {
        panic("bitmap entry not set!?");
    }

    bitmap_unmark(disk_map, index);

    lock_release(disk_map_lock);
    return;
}