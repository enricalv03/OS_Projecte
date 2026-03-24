#ifndef PMM_H
#define PMM_H

// Physical Memory Manager function declarations
// These are assembly functions, so this header is mainly for documentation

// Initialize the physical memory manager
// Sets up the bitmap and marks the bitmap page as used
void pmm_init(void);

// Allocate one physical page (4 KB)
// Returns: Physical address of allocated page, or 0 if out of memory
unsigned int pmm_alloc_page(void);

// Free one physical page
// Input: phys_addr - Physical address of page to free (must be page-aligned)
// Returns: 1 on success, 0 on error (invalid address)
unsigned int pmm_free_page(unsigned int phys_addr);

#endif

