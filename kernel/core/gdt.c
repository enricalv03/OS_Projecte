#include "gdt.h"
#include "lib/kstring.h"

/* =============================================================================
 * GDT (Global Descriptor Table) and TSS Setup
 * =============================================================================
 * Replaces the minimal 3-entry GDT set up by stage2.asm with a full GDT that
 * includes ring-3 user-mode segments and a Task State Segment (TSS).
 *
 * The TSS is required by x86 for automatic stack switching when the CPU
 * transitions from ring 3 to ring 0 on interrupts and system calls.
 * =========================================================================== */

/* GDT entry structure */
typedef struct {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;    /* flags (4 bits) + limit_high (4 bits) */
    unsigned char  base_high;
} __attribute__((packed)) gdt_entry_t;

/* GDTR structure */
typedef struct {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed)) gdt_ptr_t;

/* 6 entries: null, kernel CS, kernel DS, user CS, user DS, TSS */
#define GDT_ENTRIES 6

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdtr;
static tss_entry_t tss;

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static void gdt_set_entry(int index, unsigned int base, unsigned int limit,
                           unsigned char access, unsigned char flags) {
    gdt[index].base_low    = (unsigned short)(base & 0xFFFF);
    gdt[index].base_middle = (unsigned char)((base >> 16) & 0xFF);
    gdt[index].base_high   = (unsigned char)((base >> 24) & 0xFF);

    gdt[index].limit_low   = (unsigned short)(limit & 0xFFFF);
    gdt[index].granularity  = (unsigned char)((flags & 0xF0)
                             | ((limit >> 16) & 0x0F));
    gdt[index].access      = access;
}

/* --------------------------------------------------------------------------
 * Assembly helpers -- flush GDT and load TSS
 * -------------------------------------------------------------------------- */

/* Defined in gdt_flush.asm */
extern void gdt_flush(unsigned int gdtr_ptr);
extern void tss_flush(unsigned int tss_selector);

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void gdt_init(void) {
    /* ---------------------------------------------------------------
     * Entry 0: Null descriptor (required by x86)
     * --------------------------------------------------------------- */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* ---------------------------------------------------------------
     * Entry 1: Kernel Code Segment  (selector 0x08)
     *   Base=0, Limit=4GB, DPL=0, Execute/Read
     *   Access: 0x9A = 1 00 1 1010
     *     Present=1, DPL=00, S=1 (code/data), Exec=1, DC=0, RW=1, A=0
     *   Flags+limit_hi: 0xCF = Gran=1 (4K pages), Sz=1 (32-bit), limit[19:16]=F
     * --------------------------------------------------------------- */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);

    /* ---------------------------------------------------------------
     * Entry 2: Kernel Data Segment  (selector 0x10)
     *   Base=0, Limit=4GB, DPL=0, Read/Write
     *   Access: 0x92
     * --------------------------------------------------------------- */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);

    /* ---------------------------------------------------------------
     * Entry 3: User Code Segment  (selector 0x18, RPL 3 → 0x1B)
     *   Base=0, Limit=4GB, DPL=3, Execute/Read
     *   Access: 0xFA = 1 11 1 1010
     * --------------------------------------------------------------- */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);

    /* ---------------------------------------------------------------
     * Entry 4: User Data Segment  (selector 0x20, RPL 3 → 0x23)
     *   Base=0, Limit=4GB, DPL=3, Read/Write
     *   Access: 0xF2 = 1 11 1 0010
     * --------------------------------------------------------------- */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);

    /* ---------------------------------------------------------------
     * Entry 5: TSS (Task State Segment)  (selector 0x28)
     *   Base = address of tss structure
     *   Limit = sizeof(tss) - 1
     *   Access: 0x89 = 1 00 0 1001
     *     Present=1, DPL=0, S=0 (system), Type=1001 (32-bit TSS available)
     *   Flags: 0x00 (byte granularity, 16-bit limit is enough)
     * --------------------------------------------------------------- */
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = GDT_KERNEL_DS;    /* kernel data segment */
    tss.esp0 = 0x90000;         /* initial kernel stack (same as boot) */
    tss.iomap_base = sizeof(tss_entry_t);  /* no I/O bitmap */

    unsigned int tss_base  = (unsigned int)&tss;
    unsigned int tss_limit = sizeof(tss_entry_t) - 1;
    gdt_set_entry(5, tss_base, tss_limit, 0x89, 0x00);

    /* Load the new GDT */
    gdtr.limit = (unsigned short)(sizeof(gdt) - 1);
    gdtr.base  = (unsigned int)&gdt;
    gdt_flush((unsigned int)&gdtr);

    /* Load the TSS into the task register */
    tss_flush(GDT_TSS_SEL);
}

void tss_set_kernel_stack(unsigned int esp0) {
    tss.esp0 = esp0;
}
