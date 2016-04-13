#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <setjmp.h>
#include <thread.h>
#include <current.h>
#include <vm.h>
#include <copyinout.h>
#include <pagetable.h>

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
	return pagetable;
}

void pagetable_destroy() 
{
/* TODO Write this */
}


