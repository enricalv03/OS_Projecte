/* ============================================================================
 * arch/arm/kernel/memory/arm_pmm.c — Physical Memory Manager for ARM
 * ============================================================================
 * Simple bitmap PMM for the QEMU -M virt board (128 MB RAM at 0x40000000).
 * Reserves the first 2 MB for kernel text/data/stack and the PMM bitmap itself.
 * Each bit in the bitmap represents one 4 KB page.
 * =========================================================================== */

#include "arm_pmm.h"

#define ARM_RAM_BASE   0x40000000u
#define ARM_RAM_SIZE   (128u * 1024u * 1024u)   /* 128 MB */
#define PAGE_SIZE      4096u
#define TOTAL_PAGES    (ARM_RAM_SIZE / PAGE_SIZE) /* 32768 pages */

/* Bitmap: 1 bit per page, 1 = free, 0 = used.
 * 32768 bits = 4096 bytes = 1 page for the bitmap itself. */
#define BITMAP_WORDS   (TOTAL_PAGES / 32u)       /* 1024 dwords */

static unsigned int bitmap[BITMAP_WORDS];
static unsigned int free_pages = 0;

/* Mark a page as free (1) */
static void set_free(unsigned int page_idx) {
    bitmap[page_idx / 32] |= (1u << (page_idx % 32));
    free_pages++;
}

/* Mark a page as used (0) */
static void set_used(unsigned int page_idx) {
    bitmap[page_idx / 32] &= ~(1u << (page_idx % 32));
    if (free_pages > 0) free_pages--;
}

void arm_pmm_init(void) {
    /* Mark everything as used first */
    for (unsigned int i = 0; i < BITMAP_WORDS; i++) bitmap[i] = 0;
    free_pages = 0;

    /* Free all pages from 2 MB offset onward (leave kernel + bitmap area used) */
    unsigned int reserved_pages = (2u * 1024u * 1024u) / PAGE_SIZE;  /* 512 pages */
    for (unsigned int i = reserved_pages; i < TOTAL_PAGES; i++) {
        set_free(i);
    }
}

/* Allocate one 4 KB physical page.
 * Returns physical address, or 0 on failure. */
unsigned int arm_pmm_alloc(void) {
    for (unsigned int w = 0; w < BITMAP_WORDS; w++) {
        if (bitmap[w] == 0) continue;
        for (unsigned int b = 0; b < 32; b++) {
            if (bitmap[w] & (1u << b)) {
                unsigned int idx = w * 32 + b;
                set_used(idx);
                return ARM_RAM_BASE + idx * PAGE_SIZE;
            }
        }
    }
    return 0;
}

/* Free a previously allocated page. */
void arm_pmm_free(unsigned int phys_addr) {
    if (phys_addr < ARM_RAM_BASE) return;
    unsigned int idx = (phys_addr - ARM_RAM_BASE) / PAGE_SIZE;
    if (idx >= TOTAL_PAGES) return;
    set_free(idx);
}

unsigned int arm_pmm_free_count(void) { return free_pages; }
