#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <setjmp.h>
#include <thread.h>
#include <current.h>
#include <vm.h>
#include <copyinout.h>
#include <pagetable.h>
#include <addrspace.h>
#include <swap.h>

/*	Implementation of two-level page tables 
	First 10 bits of virtual address points to the first level page table.
	Middle 10 bits of virtual address points to the second level page table.
	Last 12 bits are offset into the page.

	pt_create()				-	Creates a page table.  
	pt_destroy()			-	
	pt_alloc_page()			-	Given a virtual address and an address space, allocate a physical page to it.			
	pt_dealloc_page()		-	
*/








struct pte* pt_load_page(struct addrspace * as, vaddr_t vaddr) 
{
	if(swap_enabled == false)
		panic("pt_load_page: swapping not enabled");

	struct pte *target = pte_get(as,vaddr);

	KASSERT(target->paddr>0);
	KASSERT(target->in_memory == 0);

	vaddr_t va = page_alloc(as,vaddr);

	target->vaddr = vaddr;
	KASSERT(va!=0);

	target->paddr = KVADDR_TO_PADDR(va);
	coremap[PADDR_TO_CM(target->paddr)].state = CLEAN; // Swapped in pages are clean!
	coremap[PADDR_TO_CM(target->paddr)].page = target;    
	int result = swapin(target->paddr,target->location_on_disk);
	(void)result;
	target -> in_memory = 1;
	KASSERT(target->in_memory == 1);
	return target;
	
}



void pt_dealloc_page(struct addrspace *as, struct  pte *target) {

	//uint32_t tlt_index = vaddr >> 22; // First 10 bits are pointer into the TLT 
	//uint32_t slt_index = vaddr >> 12 & 0x000003FF; // Second 10 bits are pointer into the SLT 

	//struct pte *target = &as->pagetable[tlt_index][slt_index];



	if(swap_enabled == false) {
		if (target -> paddr!=0) {
			page_free(as,target->paddr);
		}

	}
	else {
		
		lock_acquire(target->pte_lock);


		if(target->in_memory == 1) {

			while (true) {
				if(target->in_memory == 0) {
					deallocate_disk_index(target->location_on_disk);

					lock_release(target->pte_lock);
					break;

				}

				if(coremap[PADDR_TO_CM(target->paddr)].pinned == 0 && coremap[PADDR_TO_CM(target->paddr)].state != VICTIM) { // If coremap is not pinned.
					deallocate_disk_index(target->location_on_disk);
					page_free(as,target->paddr);
					target->paddr = 0;
					target->location_on_disk = 0;
					target->in_memory = 0;
					lock_release(target->pte_lock);
					break;

				}
				else {
					//kprintf("waiting");
					lock_release(target->pte_lock);
					thread_yield();
					lock_acquire(target->pte_lock);

				}
			}


		}
		else {
			deallocate_disk_index(target->location_on_disk);

			lock_release(target->pte_lock);
		}


	}
}


void pagetable_destroy(struct addrspace *as) {

	struct pte *target;
	int num_of_ptes = array_num(as->pagetable);
	int i = num_of_ptes - 1;


	while(i>=0){
		target = array_get(as->pagetable, i);
		if(target->paddr!=0 && target->paddr!=1) {
			 pt_dealloc_page(as, target);
		}
		//deallocate_disk_index(target->location_on_disk);
		struct lock* old_lock = target->pte_lock;
		lock_acquire(old_lock);
		kfree(target);
		lock_release(old_lock);
		lock_destroy(old_lock);
		array_remove(as->pagetable, i);
		i --;
	}	
	

    array_destroy(as->pagetable);
	

}




struct pte*
pte_get(struct addrspace *as, vaddr_t va)
{

	int i;
	struct pte *target;
	if(as == NULL) {
		panic("pte_get: addrspace is NULL");
	}
	if(as->pagetable == NULL) {
		panic("pte_get: addrspace->pagetable is NULL");
	}

	int num_of_ptes = array_num(as->pagetable);

	for (i = 0; i < num_of_ptes; i++){
		target = array_get(as->pagetable, i);
		if (va == target->vaddr){
			return target;
		}
	}
	return NULL;
}

struct pte*
pt_alloc_page(struct addrspace *as, vaddr_t va)
{
	struct pte * target = kmalloc(sizeof(struct pte));
	target->vaddr = va;
	target->location_on_disk = allocate_disk_index();
	target->pte_lock = lock_create("pte_lock");
	array_add(as->pagetable,target,NULL);

	vaddr_t va1 = page_alloc(as,va);
	if(va1==0) {
		if(swap_enabled == true) {
			panic("pt_alloc_page: swap subsystem is broken");
		}
		else {
			panic("pt_alloc_page: no memory for user pages");
		}

	}
	target->in_memory = 1;
	target->paddr = KVADDR_TO_PADDR(va1);	
	coremap[PADDR_TO_CM(target->paddr)].state = DIRTY; // Allocated pages are clean!	
	coremap[PADDR_TO_CM(target->paddr)].page = target;    




	return target;
}




int pagetable_copy(struct addrspace *old, struct addrspace *new) {

	struct pte * old_pte;
	struct pte * new_pte;
	int errno;
	int no_of_ptes = array_num(old->pagetable);
	for (int i = 0; i < no_of_ptes; i++) {
		old_pte = array_get(old->pagetable, i);
		new_pte = kmalloc(sizeof(struct pte));
		if(new_pte == NULL)
			return ENOMEM;
		new_pte->pte_lock = lock_create("pte_lock");
		if(new_pte->pte_lock ==NULL)
			return ENOMEM;

		if(swap_enabled == false) {
			vaddr_t va = page_alloc(new,old_pte->vaddr);
			if(va==0) {
				regions_destroy(new);
				pagetable_destroy(new);
    			kfree(new);
				return ENOMEM;
			}
			new_pte->paddr = KVADDR_TO_PADDR(va);
			memmove((void *)PADDR_TO_KVADDR(new_pte->paddr),(void *)PADDR_TO_KVADDR(old_pte->paddr), PAGE_SIZE);
			new_pte->vaddr = old_pte->vaddr;	
			new_pte->in_memory = 1;									
			goto add_to_pagetable;
		}
		lock_acquire(old_pte->pte_lock);

		if (old_pte == NULL || new_pte == NULL) {
			pagetable_destroy(new);
			kfree(new);
			return ENOMEM;
		}

		new_pte->location_on_disk = allocate_disk_index();
		if(old_pte->in_memory == 0 ) {
			lock_acquire(page_buffer_lock);
			int result = disk_read((vaddr_t)page_buffer,old_pte->location_on_disk);
			(void)result;
			disk_write((vaddr_t)page_buffer,new_pte->location_on_disk);
			lock_release(page_buffer_lock);


		}
		else {
			disk_write(PADDR_TO_KVADDR(old_pte->paddr),new_pte->location_on_disk);
		}

		new_pte->paddr = 1; // Lousy hack. This will be overwritten by pt_load_page anyway.
		new_pte->in_memory = 0;		
		new_pte->vaddr = old_pte->vaddr;		

		add_to_pagetable:
		errno = array_add(new->pagetable, new_pte, NULL);
		if (errno) {
			pagetable_destroy(new);
			kfree(new);

			return ENOMEM;
		}
		if(swap_enabled == true)
			lock_release(old_pte->pte_lock);
	}

	return 0;

}