/* ============================================================================
 * arch/arm/kernel/memory/arm_vmm.c — Virtual Memory Manager for ARMv7-A
 * ============================================================================
 * Implements a minimal ARMv7 MMU setup using a single-level (section-mapped)
 * page table (L1 table with 4096 entries covering 4 GB @ 1 MB / section).
 *
 * Mapping strategy (for early boot / QEMU -M virt):
 *   - Identity-map the first 128 MB of RAM (0x40000000 - 0x47FFFFFF)
 *     as kernel RW sections.
 *   - Identity-map the PL011 UART region (0x09000000) for early console.
 *   - Everything else is unmapped (fault on access).
 *
 * This is equivalent to "no real virtual address space" — it simply enables
 * the MMU so that cache + TLB hardware works correctly.  Per-process page
 * tables will be added in a future step.
 * =========================================================================== */

#include "arm_vmm.h"
#include "arm_pmm.h"

/* L1 Page-Table Descriptor (Section entry):
 *   [31:20] Physical base address of 1 MB section
 *   [19]    NS (non-secure) = 0
 *   [18]    0
 *   [17]    nG (not-global) = 0
 *   [16]    S  (shareable)  = 1  for device/normal
 *   [15]    AP[2]           = 0 (see AP field)
 *   [14:12] TEX             = 001 for normal, cacheable
 *   [11:10] AP[1:0]         = 11 (full access)
 *   [9]     P               = 0
 *   [8:5]   Domain          = 0 (use domain 0 as Manager)
 *   [4]     XN              = 0
 *   [3]     C               = 1 (cacheable)
 *   [2]     B               = 0
 *   [1:0]   = 10 (section descriptor)
 */
#define L1_SECTION_FLAGS   0x00000C0E   /* AP=11, TEX=001, C=1, B=0, section */
#define L1_DEVICE_FLAGS    0x00000C16   /* AP=11, TEX=000, C=0, B=1, section (device) */

/* L1 table: 4096 entries × 4 bytes = 16 KB, must be 16 KB-aligned */
static unsigned int __attribute__((aligned(16384))) l1_table[4096];

static void map_section(unsigned int vaddr, unsigned int paddr, unsigned int flags) {
    unsigned int idx = vaddr >> 20;
    l1_table[idx] = (paddr & 0xFFF00000u) | flags;
}

void arm_vmm_init(void) {
    /* Clear the L1 table */
    for (unsigned int i = 0; i < 4096; i++) l1_table[i] = 0;

    /* Identity-map first 128 MB of RAM (0x40000000 - 0x47FFFFFF) */
    for (unsigned int mb = 0; mb < 128; mb++) {
        unsigned int addr = 0x40000000u + (mb << 20);
        map_section(addr, addr, L1_SECTION_FLAGS);
    }

    /* Map QEMU virt UART (PL011) at 0x09000000 as device memory */
    map_section(0x09000000u, 0x09000000u, L1_DEVICE_FLAGS);

    /* Map GIC distributor (0x08000000) and CPU interface (0x08010000) */
    map_section(0x08000000u, 0x08000000u, L1_DEVICE_FLAGS);

    /* Write Translation Table Base Register 0 (TTBR0) */
    unsigned int ttbr0 = (unsigned int)l1_table;
    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 0"   /* TTBR0 = l1_table */
        :: "r"(ttbr0) : "memory"
    );

    /* Set Translation Table Base Control Register:
     * Use TTBR0 for all translations (N=0) */
    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 2"   /* TTBCR = 0 */
        :: "r"(0u) : "memory"
    );

    /* Set Domain Access Control Register:
     * Domain 0 = Manager (bypass permissions) */
    __asm__ volatile(
        "mcr p15, 0, %0, c3, c0, 0"   /* DACR = 0x3 (dom 0 = Manager) */
        :: "r"(0x3u) : "memory"
    );

    /* Invalidate TLB */
    __asm__ volatile(
        "mcr p15, 0, %0, c8, c7, 0"   /* TLBIALL */
        :: "r"(0u) : "memory"
    );

    /* Enable MMU (SCTLR bit 0), D-cache (bit 2), I-cache (bit 12) */
    unsigned int sctlr;
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr |= (1u << 0) | (1u << 2) | (1u << 12);
    __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr) : "memory");

    /* ISB to ensure pipeline flush after MMU enable */
    __asm__ volatile("isb" ::: "memory");
}
