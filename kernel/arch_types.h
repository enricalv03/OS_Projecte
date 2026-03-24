#ifndef KERNEL_ARCH_TYPES_H
#define KERNEL_ARCH_TYPES_H

/* =============================================================================
 * arch_context_t — architecture-agnostic CPU register save area
 * =============================================================================
 * Generic kernel code (scheduler, process manager) must use the fields of
 * this union/struct rather than arch-specific register names (eax, r0, etc.).
 *
 * Each architecture maps its registers onto the common alias names:
 *
 *   Arch   | regs[0] | regs[1] | regs[2] | regs[3] | ... | sp      | pc      | flags
 *   -------|---------|---------|---------|---------|-----|---------|---------|------
 *   x86    | EAX     | EBX     | ECX     | EDX     | ... | ESP     | EIP     | EFLAGS
 *   ARM    | r0      | r1      | r2      | r3      | ... | r13(sp) | r15(pc) | CPSR
 *   RISC-V | x0/ra   | a0      | a1      | a2      | ... | sp      | pc      | mstatus
 *
 * The named aliases below map to the first 8 general-purpose registers.
 * arch-specific PCBs can overlay this with their own field names for clarity.
 * ============================================================================= */

typedef struct {
    /* General-purpose registers (up to 16 per architecture) */
    unsigned int regs[16];

    /* Architectural special registers */
    unsigned int sp;        /* stack pointer */
    unsigned int pc;        /* program counter / instruction pointer */
    unsigned int flags;     /* status/flags register (EFLAGS / CPSR / mstatus) */

    /* Floating-point context (placeholder — not saved until FPU support added) */
    /* unsigned int fp_regs[32]; */
} arch_context_t;

/* Convenience aliases — map arch_context_t regs[] onto human-readable names.
 * Use these macros instead of hardcoded indices in portable code. */

/* x86 aliases */
#define CTX_EAX(c)    ((c)->regs[0])
#define CTX_EBX(c)    ((c)->regs[1])
#define CTX_ECX(c)    ((c)->regs[2])
#define CTX_EDX(c)    ((c)->regs[3])
#define CTX_ESI(c)    ((c)->regs[4])
#define CTX_EDI(c)    ((c)->regs[5])
#define CTX_EBP(c)    ((c)->regs[6])
#define CTX_ESP(c)    ((c)->sp)
#define CTX_EIP(c)    ((c)->pc)
#define CTX_EFLAGS(c) ((c)->flags)

/* ARM aliases */
#define CTX_R0(c)     ((c)->regs[0])
#define CTX_R1(c)     ((c)->regs[1])
#define CTX_R2(c)     ((c)->regs[2])
#define CTX_R3(c)     ((c)->regs[3])
#define CTX_R4(c)     ((c)->regs[4])
#define CTX_R5(c)     ((c)->regs[5])
#define CTX_R6(c)     ((c)->regs[6])
#define CTX_R7(c)     ((c)->regs[7])
#define CTX_R8(c)     ((c)->regs[8])
#define CTX_R9(c)     ((c)->regs[9])
#define CTX_R10(c)    ((c)->regs[10])
#define CTX_R11(c)    ((c)->regs[11])
#define CTX_R12(c)    ((c)->regs[12])
#define CTX_SP(c)     ((c)->sp)
#define CTX_LR(c)     ((c)->regs[14])
#define CTX_PC(c)     ((c)->pc)
#define CTX_CPSR(c)   ((c)->flags)

/* RISC-V aliases (RV32) */
#define CTX_RA(c)     ((c)->regs[0])    /* return address */
#define CTX_A0(c)     ((c)->regs[1])    /* function arg / return value */
#define CTX_A1(c)     ((c)->regs[2])
#define CTX_A2(c)     ((c)->regs[3])
#define CTX_A3(c)     ((c)->regs[4])
#define CTX_S0(c)     ((c)->regs[8])    /* callee-saved */
#define CTX_S1(c)     ((c)->regs[9])
#define CTX_MSTATUS(c) ((c)->flags)

#endif /* KERNEL_ARCH_TYPES_H */
