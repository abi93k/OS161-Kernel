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
		coremap[i].chunk_size = -1;
		coremap[i].owner = -1;
		coremap[i].as = NULL;

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
    			bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

				coremap[i].chunk_size = required_pages;
				coremap[i].state = DIRTY;
				coremap[i].as = NULL;

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
    		bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

			coremap[i].state = DIRTY;
			coremap[i].as = NULL;
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
    int i,index;
    paddr_t pa= KVADDR_TO_PADDR(addr);
    index=PADDR_TO_CM(pa);

    spinlock_acquire(&coremap_lock);
    int chunk_size = coremap[index].chunk_size;

    for(i=index;i<index+chunk_size;i++) {
    	bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

    	coremap[i].state=FREE;

    }

    coremap_used -= (PAGE_SIZE * coremap[index].chunk_size);

    spinlock_release(&coremap_lock);
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

	struct pte *target;
    uint32_t tlbhi, tlblo;
    int spl;

    struct addrspace* as = curproc->p_addrspace;


    int tlt_index = faultaddress >> 22;
    int slt_index = faultaddress >> 12 & 0x000003FF;
    


    if (as->pagetable[tlt_index] == NULL) {
        as->pagetable[tlt_index] = kmalloc(MAX_PTE * sizeof(struct pte));
        memset(as->pagetable[tlt_index], 0, MAX_PTE * sizeof(struct pte));
    }

	target = &as->pagetable[tlt_index][slt_index];

    if (target->paddr == 0) {
        target = pt_alloc_page(as, faultaddress & PAGE_FRAME); 	// pt_alloc_page creates the page table entry if neccessary and also
        														// allocates it using coremap.
    }

    if(target->paddr == 0) { // Seriously ?
    	return ENOMEM;
    }


    tlbhi = faultaddress & PAGE_FRAME;
    tlblo = (target->paddr & PAGE_FRAME) | TLBLO_VALID;

    spl = splhigh();
    int index;

    switch (faulttype) {
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:

            tlb_random(tlbhi, tlblo);
            break;
        case VM_FAULT_READONLY:

            tlblo |= TLBLO_DIRTY;

            index = tlb_probe(faultaddress & PAGE_FRAME, 0);
            tlb_write(tlbhi, tlblo, index);
    }

    splx(spl);

    return 0;

}



vaddr_t page_alloc(struct addrspace *as, vaddr_t vaddr) {
	(void)vaddr;
	spinlock_acquire(&coremap_lock);

	for(int i = 0; i < no_of_coremap_entries; i++) {

		if(coremap[i].state == FREE) {
			coremap[i].state = DIRTY;
			coremap[i].chunk_size = 1;
			coremap[i].as = as;
			coremap[i].owner = curproc->pid;
			
			coremap_used += PAGE_SIZE;

			spinlock_release(&coremap_lock);
			bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

			return PADDR_TO_KVADDR(CM_TO_PADDR(i));
		}


	}

	spinlock_release(&coremap_lock);

	return 0;

}

void page_free(struct addrspace *as, paddr_t paddr) {

    int i,index;
    index=PADDR_TO_CM(paddr);

    spinlock_acquire(&coremap_lock);
    int chunk_size = coremap[index].chunk_size;

    for(i=index;i<index+chunk_size;i++) {

    	bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);


    	coremap[i].state=FREE;
    }

    coremap_used -= (PAGE_SIZE * coremap[index].chunk_size);

    spinlock_release(&coremap_lock);
    (void)as;

}



