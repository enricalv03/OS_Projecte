#ifndef GDT_H
#define GDT_H

/* =============================================================================
 * GDT Segment Selectors
 * =============================================================================
 * Entry 0: Null       (0x00)
 * Entry 1: Kernel CS  (0x08)  DPL 0
 * Entry 2: Kernel DS  (0x10)  DPL 0
 * Entry 3: User CS    (0x18)  DPL 3  (selector used by ring-3: 0x1B)
 * Entry 4: User DS    (0x20)  DPL 3  (selector used by ring-3: 0x23)
 * Entry 5: TSS        (0x28)
 * =========================================================================== */

#define GDT_KERNEL_CS   0x08
#define GDT_KERNEL_DS   0x10
#define GDT_USER_CS     0x1B   /* 0x18 | RPL 3 */
#define GDT_USER_DS     0x23   /* 0x20 | RPL 3 */
#define GDT_TSS_SEL     0x28

/* TSS (Task State Segment) -- only esp0 and ss0 are used by the CPU
 * for automatic ring 0 stack switching on interrupts/syscalls. */
typedef struct tss_entry {
    unsigned int prev_tss;
    unsigned int esp0;      /* Ring-0 stack pointer (set before each user switch) */
    unsigned int ss0;       /* Ring-0 stack segment (always GDT_KERNEL_DS = 0x10) */
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax, ecx, edx, ebx;
    unsigned int esp, ebp, esi, edi;
    unsigned int es, cs, ss, ds, fs, gs;
    unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed)) tss_entry_t;

/* Initialize the GDT with kernel + user segments and TSS.
 * Must be called early in the kernel boot sequence. */
void gdt_init(void);

/* Update the TSS ring-0 stack pointer. Call this before switching to
 * a user-mode process so the CPU knows which stack to use on interrupts. */
void tss_set_kernel_stack(unsigned int esp0);

#endif
