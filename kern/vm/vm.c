#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <vm.h>

/*
 * VM system
 */

void
vm_bootstrap(void)
{
	spinlock_init(&coremap_lock);

	paddr_t fpaddr, lpaddr, new_fpaddr;
	int coremap_size;

	fpaddr = ram_getfirstfree();	
	lpaddr = ram_getsize();	

	no_of_physical_pages = (lpaddr-fpaddr) / PAGE_SIZE; // We do not consider the memory stolen by kernel during boot.
														// Should we ?
	
	coremap_size = no_of_physical_pages * sizeof(struct coremap_entry);
	coremap_size = ROUNDUP(coremap_size, PAGE_SIZE);

	coremap = (struct coremap_entry *)PADDR_TO_KVADDR(fpaddr); // Placing the coremap at first available physical address.

	new_fpaddr = fpaddr + coremap_size; // Moving my fpaddr to accomadate the coremap.

	no_of_coremap_entries = (lpaddr - new_fpaddr) / PAGE_SIZE; // Absurd to store pages containing coremap in coremap.

	free_page_start = new_fpaddr / PAGE_SIZE; // First free page. This page maps to 0th entry of coremap.

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
	/* TODO Write this */

	(void)npages;

	return 0;
}

void
free_kpages(vaddr_t addr)
{
	/* TODO Write this */

	(void)addr;
}

unsigned
int
coremap_used_bytes() 
{
	/* TODO Write this */

	return 0;
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


