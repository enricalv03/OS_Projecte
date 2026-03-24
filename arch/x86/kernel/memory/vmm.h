#ifndef VMM_H
#define VMM_H

// Virtual Memory Manager (VMM) functions
// These functions manage virtual-to-physical page mappings

// Initialize VMM (sets up kernel page directory)
void vmm_init(void);

// Map a virtual page to a physical page
// virtual_addr: virtual address (must be page-aligned, 4KB boundary)
// physical_addr: physical address (must be page-aligned)
// flags: page flags (0x03 = Present + Read/Write, 0x07 = Present + Read/Write + User)
// Returns: 1 on success, 0 on failure
int vmm_map_page(unsigned int virtual_addr, unsigned int physical_addr, unsigned int flags);

// Unmap a virtual page
// virtual_addr: virtual address to unmap
// Returns: 1 on success, 0 on failure
int vmm_unmap_page(unsigned int virtual_addr);

// Get physical address from virtual address
// virtual_addr: virtual address to translate
// Returns: physical address, or 0 if not mapped
unsigned int vmm_get_physical(unsigned int virtual_addr);

// Allocate and map a new virtual page
// virtual_addr: virtual address to map (must be page-aligned)
// flags: page flags
// Returns: physical address of allocated page, or 0 on failure
unsigned int vmm_alloc_and_map(unsigned int virtual_addr, unsigned int flags);

// Unmap and free a virtual page
// virtual_addr: virtual address to unmap and free
// Returns: 1 on success, 0 on failure
int vmm_unmap_and_free(unsigned int virtual_addr);

// Page flags
#define VMM_PRESENT   0x01  // Page is present in memory
#define VMM_WRITABLE  0x02  // Page is writable
#define VMM_USER      0x04  // Page is accessible from user mode
#define VMM_KERNEL   0x00  // Page is kernel-only (default)

// Convenience flags
#define VMM_KERNEL_RW (VMM_PRESENT | VMM_WRITABLE)
#define VMM_USER_RW   (VMM_PRESENT | VMM_WRITABLE | VMM_USER)

#endif

