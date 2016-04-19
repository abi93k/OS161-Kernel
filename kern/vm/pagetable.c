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

/*	Implementation of two-level page tables 
	First 10 bits of virtual address points to the first level page table.
	Middle 10 bits of virtual address points to the second level page table.
	Last 12 bits are offset into the page.

	pt_create()				-	Creates a page table.  
	pt_destroy()			-	
	pt_alloc_page()			-	Given a virtual address and an address space, allocate a physical page to it.			
	pt_dealloc_page()		-	
*/



struct pte ** pagetable_create() 
{
	struct pte** pagetable;
	pagetable = kmalloc(MAX_PTE * sizeof(struct pte*));
	if(pagetable == NULL) { // Temporary fix.
		return pagetable;
	}
	for (int i = 0; i < MAX_PTE; i++) {
		pagetable[i] = NULL;
	}
	return pagetable;
}

struct pte* pt_alloc_page(struct addrspace * as, vaddr_t vaddr) 
{

	uint32_t tlt_index = vaddr >> 22; // First 10 bits are pointer into the TLT 
	uint32_t slt_index = vaddr >> 12 & 0x000003FF; // Second 10 bits are pointer into the SLT 

	if(as->pagetable[tlt_index] == NULL) {
		//kprintf("%d",sizeof(struct pte));
		as->pagetable[tlt_index] = kmalloc(MAX_PTE * sizeof(struct pte));
		//memset(as->pagetable[tlt_index], 0, MAX_PTE * sizeof(struct pte));
	}

	struct pte *target = &as->pagetable[tlt_index][slt_index];

	target -> in_memory = 1;
	vaddr_t va = page_alloc(as,vaddr);
	if(va!=0) {
		target->paddr = KVADDR_TO_PADDR(va);
	}
	else{
		target->paddr = 0;
	}




	return target;
	
}


void pt_dealloc_page(struct addrspace *as, struct  pte *target) {

	//uint32_t tlt_index = vaddr >> 22; // First 10 bits are pointer into the TLT 
	//uint32_t slt_index = vaddr >> 12 & 0x000003FF; // Second 10 bits are pointer into the SLT 

	//struct pte *target = &as->pagetable[tlt_index][slt_index];

	if (target->paddr!=0) {
		page_free(as,target->paddr);
	}

	target->paddr = 0;
	target->location_on_disk = 0;
	target->in_memory = 0;

}


void pagetable_destroy(struct addrspace *as, struct pte** pagetable) {
    int i, j;
    struct pte *target;
    for (i = 0; i < MAX_PTE; i++) {
        if (pagetable[i] != NULL) {
            for (j = 0; j < MAX_PTE; j++) {
                target = &pagetable[i][j];
                if (target->paddr!=0) {
                    pt_dealloc_page(as, target);
                }
            }
       		kfree(pagetable[i]);
        }
    }
    kfree(pagetable);
}




