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
	/* TODO Write this */
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


