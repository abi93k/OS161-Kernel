#ifndef _PAGETABLE_H_
#define _PAGETABLE_H_

#include <vm.h>
#include <addrspace.h>
#include <synch.h>

struct addrspace;

struct pte {
    paddr_t paddr;			
    int location_on_disk;
    int in_memory;
    int allocated;
};

#define MAX_PTE 1024


struct pte ** pagetable_create();
void pagetable_destroy();

#endif