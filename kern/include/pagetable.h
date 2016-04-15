#include <synch.h>

//struct addrspace;

struct pte {
    paddr_t paddr;			
    int location_on_disk;
    int in_memory;
};

#define MAX_PTE 1024


struct pte ** pagetable_create(void);
void pagetable_destroy(struct addrspace *as, struct pte** pagetable);
struct pte* pt_alloc_page(struct addrspace * as, vaddr_t vaddr);
void pt_dealloc_page(struct addrspace *as, struct  pte *target);


