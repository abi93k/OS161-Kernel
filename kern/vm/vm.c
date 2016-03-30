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

/*
 * VM system
 */
  
void
vm_bootstrap(void)
{
	spinlock_init(&coremap_lock);

	paddr_t fpaddr, lpaddr;
	uint32_t coremap_size;

	lpaddr = ram_getsize();	
	fpaddr = ram_getfirstfree();	

	no_of_physical_pages = (lpaddr-fpaddr) / PAGE_SIZE; // We do not consider the memory stolen by kernel during boot.
														// Should we ?
	
	coremap_size = no_of_physical_pages * sizeof(struct coremap_entry);
	coremap_size = ROUNDUP(coremap_size, PAGE_SIZE);
	KASSERT((coremap_size & PAGE_FRAME) == coremap_size);

	coremap = (struct coremap_entry *)PADDR_TO_KVADDR(fpaddr); // Placing the coremap at first available physical address.

	fpaddr = fpaddr + coremap_size; // Moving my fpaddr to accomadate the coremap.

	no_of_coremap_entries = (lpaddr - fpaddr) / PAGE_SIZE; // Absurd to store pages containing coremap in coremap.

	free_page_start = fpaddr / PAGE_SIZE; // First free page. This page maps to 0th entry of coremap.

	for (int i=0; i<no_of_coremap_entries;i++){

		coremap[i].vm_addr = 0;
		coremap[i].state = FREE;
		coremap[i].chunk_size = 1;
		coremap[i].owner = -1;

	}

}

/* Allocate/free some kernel-space virtual pages */

vaddr_t
alloc_kpages(unsigned npages)
{
	int required_pages = (int) npages;
	int available_pages;
	int start = -1;

	spinlock_acquire(&coremap_lock);

	for(int i = 0; i < no_of_coremap_entries; i++) {
		available_pages = 0;
		if(coremap[i].state == FREE) {
			for(int j = i; j < i + required_pages; j++) {
				if(coremap[j].state == FREE) {
					available_pages ++;
				}
				else {
					break;
				}
			}
			if(available_pages == required_pages) {
				start = i;
				coremap[i].chunk_size = required_pages;
				coremap[i].state = DIRTY;

				break;
			}
		}

	}

	if(available_pages != required_pages) {
		spinlock_release(&coremap_lock);
		return 0;
	}
	else {
		for(int i = start+1; i < start + coremap[start].chunk_size; i++) {
			coremap[i].state = DIRTY;
		}
	}

	coremap_used += PAGE_SIZE * required_pages;
	spinlock_release(&coremap_lock);

	paddr_t r = CM_TO_PADDR(start);
	return PADDR_TO_KVADDR(r);
}

void
free_kpages(vaddr_t addr)
{
	/* TODO Write this */
    int i,index;
    paddr_t pa= KVADDR_TO_PADDR(addr);
    index=PADDR_TO_CM(pa);

    spinlock_acquire(&coremap_lock);
    // get chunk_size 
    int chunk_size = coremap[index].chunk_size;

    for(i=index;i<index+chunk_size;i++) {
    	coremap[i].state=FREE;

     
    }

    coremap_used -= (PAGE_SIZE * coremap[index].chunk_size);
    //addressspace object to check if mapped to userspace ?
    // needs to be unmapped if previously mapped to user mempory

    spinlock_release(&coremap_lock);
	//(void)addr;
}

unsigned
int
coremap_used_bytes() 
{
	return coremap_used;
}

void
vm_tlbshootdown_all(void)
{
	/* TODO Write this */
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	/* TODO Write this */

	(void)ts;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	/* TODO Write this */

	(void)faulttype;
	(void)faultaddress;

	return 0;
}


