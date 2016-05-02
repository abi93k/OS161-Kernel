#include <synch.h>

//struct addrspace;

struct pte {
    paddr_t paddr;	
    vaddr_t vaddr;		
    short location_on_disk;
    int in_memory;
    struct lock * pte_lock;
};

#define MAX_PTE 1024


void pagetable_destroy(struct addrspace *as);
struct pte* pte_get(struct addrspace * as, vaddr_t vaddr);
struct pte* pt_load_page(struct addrspace * as, vaddr_t vaddr);
void pt_dealloc_page(struct addrspace *as, struct  pte *target);
struct pte* pte_get(struct addrspace *as, vaddr_t va);
int pagetable_copy(struct addrspace *old, struct addrspace *new);
struct pte* pt_alloc_page(struct addrspace *as, vaddr_t va);


