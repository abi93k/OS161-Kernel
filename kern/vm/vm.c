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
#include <cpu.h>

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

		coremap[i].state = FREE;
		coremap[i].last_page = -1;
		coremap[i].as = NULL;
		coremap[i].cpu = -1;
		coremap[i].pinned = 0;
		coremap[i].page = NULL;


	}

	page_buffer_lock = lock_create("page_buffer_lock");

}

/* Allocate/free some kernel-space virtual pages */


vaddr_t
alloc_kpages(unsigned npages)
{
	if(swap_enabled == true)
		return alloc_kpages_swap(npages);
	return alloc_kpages_noswap(npages);
}

vaddr_t
alloc_kpages_swap(unsigned npages)
{
	int required_pages = (int) npages;
	int available_pages;
	int start = -1;
	int end = -1;
	int victim_states[required_pages];

	spinlock_acquire(&coremap_lock);

	for(int i = 0; i < no_of_coremap_entries; i++) {
		available_pages = 0;
		if((coremap[i].state == FREE || coremap[i].state == CLEAN || coremap[i].state == DIRTY) && coremap[i].pinned == 0 ) { // We can swap these out! :)
			for(int j = i; j < i + required_pages && j < no_of_coremap_entries; j++) {
				if((coremap[j].state == FREE || coremap[j].state == CLEAN || coremap[j].state == DIRTY) && coremap[i].pinned == 0) {
					available_pages ++;
				}
				else {
					break;
				}
				

			}
			if(available_pages == required_pages) {
				start = i;
				end = i-1;
				for(int k = 0; k < required_pages;k++) {
					victim_states[k] = coremap[start+k].state;
					coremap[start+k].state = VICTIM;
					coremap[start+k].pinned = 1;
					end++;
				}
				break;
			}
		}

	}
	spinlock_release(&coremap_lock);

	if(available_pages != required_pages) {
		
		return 0;
	}


	for(int i = start; i <=end; i++) {
		vaddr_t lock_vaddr;
		struct addrspace *lock_as;
		struct pte* target;
		if(victim_states[i - start] != FREE) {
			lock_vaddr = coremap[i].vaddr;		
			lock_as = coremap[i].as;
			if(lock_as == NULL) {
				panic("not supoosed to be null");
			}
			//target = pte_get(lock_as,lock_vaddr);	
			target =  coremap[i].page; // O(1)
			if(target == NULL) {
				kprintf("\n %d \n",lock_vaddr);
				kprintf("\n %p \n",lock_as);
				kprintf("\n %d \n",coremap[i].state);
				kprintf("\n %d \n",coremap[i].last_page);
				kprintf("\n %d \n",coremap[i].pinned);
				kprintf("\n %d \n",coremap[i].cpu);
			}
			lock_acquire(target->pte_lock);


			npages--;

			KASSERT(lock_as->pagetable!=NULL);
			KASSERT(lock_as!=NULL);
			KASSERT(target!=NULL);


			if(target->in_memory == 1) { // Check if still in memory 
				if(coremap[i].state == VICTIM)
					MAKE_PAGE_AVAIL(i,victim_states[i - start]); // new state
				else
					MAKE_PAGE_AVAIL(i,coremap[i].state); // new state


			}
		}


    	//KASSERT(coremap[i].state == VICTIM);
        bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);
	
		coremap[i].as = NULL;
		coremap[i].vaddr = PADDR_TO_KVADDR(CM_TO_PADDR(i));
		coremap[i].state = FIXED;
		coremap[i].last_page = 0;
		coremap[i].cpu = -1;
		coremap[i].pinned = 0;
		coremap[i].page = NULL;

		if(i == end) {
			coremap[i].last_page = 1;

		}
		if(victim_states[i - start] != FREE) {		
			lock_release(target->pte_lock);
		}
	
	}


	spinlock_acquire(&coremap_lock);

	coremap_used += PAGE_SIZE * npages;
	spinlock_release(&coremap_lock);


	paddr_t r = CM_TO_PADDR(start);

	return PADDR_TO_KVADDR(r);
}


vaddr_t
alloc_kpages_noswap(unsigned npages)
{
	int required_pages = (int) npages;
	int available_pages;
	int start = -1;
	int end = -1;

	spinlock_acquire(&coremap_lock);

	for(int i = 0; i < no_of_coremap_entries; i++) {
		available_pages = 0;
		if(coremap[i].state == FREE) {
			for(int j = i; j < i + required_pages; j++) {
				if(j<no_of_coremap_entries) { // Hopefully fixes Bus Errors. 
					if(coremap[j].state == FREE) {
						available_pages ++;
					}
					else {
						break;
					}
				}
				else {
					break;

				}
			}
			if(available_pages == required_pages) {
				start = i;
				end = i;
    			bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

				coremap[i].state = FIXED;
				coremap[i].as = NULL;
				coremap[i].last_page = 0;
				coremap[i].cpu = -1;



				break;
			}
		}

	}

	if(available_pages != required_pages) {
		spinlock_release(&coremap_lock);
		return 0;
	}
	else {
		for(int i = start+1; i < start + required_pages; i++) {
    		bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

			coremap[i].state = FIXED;
			coremap[i].as = NULL;
			coremap[i].last_page = 0;
			end++;

		}
	}

	coremap[end].last_page = 1;

	coremap_used += PAGE_SIZE * required_pages;
	spinlock_release(&coremap_lock);

	paddr_t r = CM_TO_PADDR(start);
	return PADDR_TO_KVADDR(r);
}

void
free_kpages(vaddr_t addr)
{
    int i,index;
    int number_of_pages_deallocated=0;
    paddr_t pa= KVADDR_TO_PADDR(addr);
    index=PADDR_TO_CM(pa);

    spinlock_acquire(&coremap_lock);

    for(i=index;i<no_of_coremap_entries;i++) {
    	//bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);
    	int backup_last_page = coremap[i].last_page;
		coremap[i].state = FREE;
		coremap[i].last_page = -1;
		coremap[i].as = NULL;
		coremap[i].vaddr = 0;
		coremap[i].cpu = -1;
		coremap[i].pinned = 0;

		
		number_of_pages_deallocated++;

		if(backup_last_page)
			break;

    }

    coremap_used -= (PAGE_SIZE * number_of_pages_deallocated);

    spinlock_release(&coremap_lock);
}

unsigned
int
coremap_used_bytes() 
{
	return coremap_used;
}

void
vm_tlbflush_all(void)
{
	int i, spl;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
	splx(spl);
}

void 
vm_tlbflush(vaddr_t target) 
{
    int spl;
    int index;

    spl = splhigh();
    index = tlb_probe(target & PAGE_FRAME, 0);
    if (index >=0) 
        tlb_write(TLBHI_INVALID(index), TLBLO_INVALID(), index);
    splx(spl);
}

void
vm_tlbshootdown_all(void)
{
	/* TODO Write this */
}


void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	vm_tlbflush(ts->target);
    V(ts->sem);
}



int
vm_fault(int faulttype, vaddr_t faultaddress)
{

	struct pte *target;
    uint32_t tlbhi, tlblo;
    int spl;

    struct addrspace* as = curproc->p_addrspace;

    int permission = as_check_region(as, faultaddress);
    

    if (permission < 0 // check if not in region
    	&& as_check_stack(as,faultaddress) // check if not in stack
    	&& as_check_heap(as,faultaddress)) // check if not in heap
        	return EFAULT;
    
    if(permission<0)
    	permission = READ | WRITE;


    target = pte_get(as,faultaddress & PAGE_FRAME);

    if(target==NULL) {
    	target = pt_alloc_page(as,faultaddress & PAGE_FRAME);
    }
	

    // Lock pagetable entry
    if(swap_enabled == true)
    	lock_acquire(target->pte_lock);


    
    if(target->in_memory == 0) { // Page is allocated but not in memory.

    	target = pt_load_page(as, faultaddress & PAGE_FRAME); 	// pt_alloc_page creates the page table entry if neccessary and also
    	    													// allocates it using coremap.    	
    }
    



	KASSERT(target->in_memory != 0);
	KASSERT(target->paddr != 0);
	KASSERT(target->paddr != 1);
	
    tlbhi = faultaddress & PAGE_FRAME;
    tlblo = (target->paddr & PAGE_FRAME) | TLBLO_VALID;

    /* Adding CPU index to corresponding TLB entry */

    coremap[PADDR_TO_CM(target->paddr)].cpu = curcpu->c_number;    
    //coremap[PADDR_TO_CM(target->paddr)].page = target;    
    int index;

    spl = splhigh();

    // TODO permissions
    //kprintf(" \n %x - %x \n",tlbhi, tlblo);
    switch (faulttype) {
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:


            //index = PADDR_TO_CM(target->paddr);
            //coremap[index].state = DIRTY;
            tlb_random(tlbhi, tlblo);
            break;
        case VM_FAULT_READONLY:

            tlblo |= TLBLO_DIRTY;
            // TODO: Change physical page's state to DIRTY.
            index = PADDR_TO_CM(target->paddr);
            //KASSERT(coremap[index].state!=FIXED);
            //KASSERT(coremap[index].state!=VICTIM);
            KASSERT(target->in_memory != 0); // Someone swapped me out. Synchronization is broken.
            //KASSERT(coremap[index].as ==as);
            coremap[index].state = DIRTY; // Set it to dirty!


            index = tlb_probe(faultaddress & PAGE_FRAME, 0);
            tlb_write(tlbhi, tlblo, index);
    }

    splx(spl);
    if(swap_enabled == true)
    	lock_release(target->pte_lock);

    return 0;

}



vaddr_t page_alloc(struct addrspace *as, vaddr_t vaddr) {
	spinlock_acquire(&coremap_lock);

	for(int i = no_of_coremap_entries-1; i >= 0; i--) {

		if(coremap[i].state == FREE) {
			coremap[i].state = DIRTY;
			coremap[i].last_page = 1;
			KASSERT(as!=NULL); // User process has no address space ?!
			coremap[i].as = as;
			coremap[i].vaddr = vaddr;
			coremap[i].cpu = -1;
			coremap[i].pinned = 0;
			coremap[i].page = NULL;

			
			coremap_used += PAGE_SIZE;

			spinlock_release(&coremap_lock);
			bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);

			return PADDR_TO_KVADDR(CM_TO_PADDR(i));
		}


	}
	// Reached here only if we run out of coremap entries, need to swap.
	if(swap_enabled == false) {
		spinlock_release(&coremap_lock);		
		return 0;
	}
	int victim_index = choose_victim();
	KASSERT(victim_index>=0);
	int previous_state = coremap[victim_index].state;


    struct addrspace *lock_as = coremap[victim_index].as;
    vaddr_t lock_vaddr = coremap[victim_index].vaddr;
    KASSERT(coremap[victim_index].state != FREE);

    coremap[victim_index].state = VICTIM; // Fuck, missed this!
    coremap[victim_index].pinned = 1; // Fuck, missed this!
    KASSERT(lock_as != NULL); // WHAT ?!
    KASSERT(lock_as->pagetable!=NULL);

    //struct pte* target = pte_get(lock_as,lock_vaddr);
    struct pte* target =  coremap[victim_index].page; // O(1)
    if(target == NULL) {
		kprintf("\n %d \n",lock_vaddr);
		kprintf("\n %d \n",(int)lock_as);
	}
    KASSERT(target->pte_lock!=NULL);
	spinlock_release(&coremap_lock);
	//KASSERT(coremap[victim_index].state == VICTIM);



    lock_acquire(target->pte_lock);

	KASSERT(coremap[victim_index].pinned == 1);

	if(target -> in_memory == 1) { // Pagetable destroy.
		if(coremap[victim_index].state != VICTIM)
			previous_state = coremap[victim_index].state;

		MAKE_PAGE_AVAIL(victim_index,previous_state);
	}

 
	coremap[victim_index].state = VICTIM;
	coremap[victim_index].as = as; // new as
	coremap[victim_index].vaddr = vaddr; // new vaddr;
	coremap[victim_index].last_page = 1;
	coremap[victim_index].pinned = 0;
	coremap[victim_index].page = NULL;
	

	bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(victim_index)), PAGE_SIZE);

	lock_release(target->pte_lock);


	return PADDR_TO_KVADDR(CM_TO_PADDR(victim_index));
	

}

void page_free(struct addrspace *as, paddr_t paddr) 
{
	(void)as;
    int i,index;
    int number_of_pages_deallocated=0;
    index=PADDR_TO_CM(paddr);

    spinlock_acquire(&coremap_lock);

    for(i=index;i<no_of_coremap_entries;i++) {
    	//bzero((void *)PADDR_TO_KVADDR(CM_TO_PADDR(i)), PAGE_SIZE);
    	int backup_last_page = coremap[i].last_page;
		coremap[i].state = FREE;
		coremap[i].last_page = -1;
		coremap[i].as = NULL;
		coremap[i].vaddr = 0;
		coremap[i].cpu = -1;
		coremap[i].pinned = 0;
		coremap[i].page = NULL;


		
		number_of_pages_deallocated++;

		if(backup_last_page)
			break;

    }

    coremap_used -= (PAGE_SIZE * number_of_pages_deallocated);

    spinlock_release(&coremap_lock);

}

void MAKE_PAGE_AVAIL(int coremap_index,int previous_state) {
	if(previous_state == FREE) {
		// Do nothing!
	}
	else if(previous_state == CLEAN) {

		vaddr_t vaddr = coremap[coremap_index].vaddr;
		unsigned cpu = coremap[coremap_index].cpu;
		ipi_tlbshootdown_allcpus(&(const struct tlbshootdown){vaddr, tlb_sem, cpu});

		//struct pte* target = pte_get(coremap[coremap_index].as,vaddr);
		struct pte* target =  coremap[coremap_index].page; // O(1)
		KASSERT(target!=NULL);

		target->in_memory = 0;
	}

	else if(previous_state == DIRTY) {


		int result = swapout(coremap_index);
		(void)result;


	}
	else if(previous_state == VICTIM) {
		panic("Victim is pinnned!");

	}
	else if(previous_state == FIXED) {
		panic("Victim is FIXED!");

	}
}



