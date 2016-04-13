#include <synch.h>

struct addrspace;

struct pte {
    paddr_t paddr;			
    int location_on_disk;
    int in_memory;
    int allocated;
};

#define MAX_PTE 1024


struct pte ** pagetable_create(void);
void pagetable_destroy(void);

