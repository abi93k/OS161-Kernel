/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <array.h>
#include <spl.h>
#include <mips/tlb.h>
#include <pagetable.h>


/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */


struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */

	as->pagetable=pagetable_create();
	as->regions=array_create();
	as->heap_start=0;
	as->heap_end=0;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	newas->heap_start = old->heap_start;
	newas->heap_end = old->heap_end;

	// copy regions

	struct region * old_region;
	struct region * new_region;

	int no_of_regions = array_num(old->regions);
	for (int i = 0; i < no_of_regions; i++) {
		old_region = array_get(old->regions, i);
		new_region = kmalloc(sizeof(struct region));
		if (old_region == NULL || new_region == NULL) {
			regions_cleanup(newas,no_of_regions);
			return ENOMEM;
		}

		new_region->base = old_region->base;
		new_region->size = old_region->size;
		new_region->permission = old_region->permission;

		int errno = array_add(newas->regions, new_region, NULL);
		if (errno) {
			regions_cleanup(newas,no_of_regions);
			return ENOMEM;
		}
	}

	// copy page table


	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */

    // TODO Destroy page table.

    int num_of_regions = array_num(as->regions);
    struct region *region_ptr;

    int i = num_of_regions - 1;

    while (i >= 0){
        region_ptr = array_get(as->regions, i);
        kfree(region_ptr);
        array_remove(as->regions, i);
        i--;
    }

    array_destroy(as->regions);

    kfree(as);

}

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/*
	 * Write this.
	 */

	int i, spl;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
	
	splx(spl);
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */

	 /* Not required */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	struct region *region;

	/* Align the region. First, the base... */
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;

	region = kmalloc(sizeof(struct region));

	if (region == NULL)
		return ENOMEM;

	region->base = vaddr;
	region->size = memsize;
	region->permission = readable + writeable + executable;

	array_add(as->regions, region, NULL);

	as->heap_start = vaddr + memsize;
	as->heap_end = as->heap_start;

	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */	

	 int num_of_regions=(int)array_num(as->regions);
	 struct region * region_ptr;

	 for(int i=0;i<num_of_regions;i++)
	 {
	 	region_ptr = (struct region *)array_get(as->regions,i);
	 	region_ptr->permission= READ | WRITE;
	 }

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	 
	 int num_of_regions=(int)array_num(as->regions);
	 struct region * region_ptr;

	 for(int i=0;i<num_of_regions;i++)
	 {
	 	region_ptr = (struct region *)array_get(as->regions,i);
	 	region_ptr->permission = region_ptr->original_permission;
	 }

	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	*stackptr = USERSTACK;

	(void)as;

	return 0;
}


/* helper */

void regions_cleanup(struct addrspace *as, int no_of_regions) {
		struct region *region_ptr;
		for (int i=0; i < no_of_regions; i++){

			region_ptr = (struct region *)array_get(as->regions,i);
			if (region_ptr != NULL)
				kfree(region_ptr);
			}
}

