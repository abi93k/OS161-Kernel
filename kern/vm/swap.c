

int choose_victim(/*Add as necessary*/) {
	/* TODO Write this */

	// Select a page which is NOT fixed
	// Return its index.
	// Panic if no page can be selected.
}

int swapout(/*Add as necessary*/) {
	/* TODO Write this */

	//Write a dirty page to disk.
	//Return 0 on success
	//Return 1 on failure
}

int swapin(/*Add as necessary*/) {
	/* TODO Write this */

	//Read a page from disk to physical memory.
	//Mark relevant page table entry.
	//Return 0 on success
	//Return 1 on failure
}

int swap(/*Add as necessary*/) {
	/* TODO Write this */

	// choose_victim
	// remove victim -> vaddr from TLB.
	// if victim->state == DIRTY
		// swap_out
		// victim->state = CLEAN

	// victim->as->pagetable[victim->vaddr~][victim->vaddr~]->in_memory = 0
	// victim ->state = FREE
	// Reduce coremap_used_bytes

	// Fill victim with new information.
	// Increase coremao_used_bytes.
	// cv_signal to continue vm_fault (cv_wait triggered when swapping starts) ?

}