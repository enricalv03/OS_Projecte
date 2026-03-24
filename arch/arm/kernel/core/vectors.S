/* ============================================================================
 * arch/arm/kernel/core/vectors.S — ARMv7 Exception Vector Table
 * ============================================================================
 * The ARM architecture requires exception vectors at address 0x00000000 (or
 * 0xFFFF0000 if HIVECS is set).  For QEMU -M virt we keep HIVECS off and
 * copy this table to 0x00000000 before enabling the MMU.
 *
 * Alternatively, set VBAR (Vector Base Address Register) to point here
 * (available on Cortex-A8+).  We use VBAR for simplicity — no copy needed.
 * ============================================================================
 *
 * Vector offsets:
 *   0x00  Reset          — not used after boot
 *   0x04  Undefined Instruction
 *   0x08  SVC (Supervisor Call) — used for syscalls
 *   0x0C  Prefetch Abort
 *   0x10  Data Abort
 *   0x14  Reserved
 *   0x18  IRQ
 *   0x1C  FIQ
 * ============================================================================ */

    .section .text.vectors
    .global  arm_vector_table

arm_vector_table:
    b  _reset_handler
    b  _undef_handler
    b  _svc_handler
    b  _prefetch_handler
    b  _data_abort_handler
    b  _reserved_handler
    b  _irq_handler
    b  _fiq_handler

/* ---- Reset handler (we should never reach here after boot) ---- */
_reset_handler:
    b  _reset_handler

/* ---- Undefined Instruction ---- */
_undef_handler:
    push  {r0-r12, lr}
    mov   r0, #1                     /* fault type: undefined instruction */
    bl    arm_fault_handler
    pop   {r0-r12, pc}

/* ---- Prefetch Abort ---- */
_prefetch_handler:
    push  {r0-r12, lr}
    mov   r0, #3
    bl    arm_fault_handler
    pop   {r0-r12, pc}

/* ---- Data Abort ---- */
_data_abort_handler:
    push  {r0-r12, lr}
    mov   r0, #4
    bl    arm_fault_handler
    pop   {r0-r12, pc}

/* ---- Reserved ---- */
_reserved_handler:
    b  _reserved_handler

/* ---- FIQ (fast interrupt) — spin for now ---- */
_fiq_handler:
    b  _fiq_handler

/* ---- SVC handler — system call via `svc #n` ---- */
    .global _svc_handler
    .extern arm_syscall_dispatch
_svc_handler:
    /* On SVC entry the CPU is in SVC mode with LR_svc = return address.
     * We need to save all user registers and set up a C call. */
    push  {r0-r12, lr}           /* save user registers + LR */
    /* r0-r3 already hold syscall args (set by caller convention).
     * The syscall number is the immediate encoded in the SVC instruction.
     * Read it from [LR - 4] (the SVC instruction itself). */
    ldr   r7, [lr, #-4]          /* r7 = SVC instruction */
    bic   r7, r7, #0xFF000000    /* r7 = SVC immediate (syscall number) */
    /* Call C dispatcher: arm_syscall_dispatch(num, r0, r1, r2, r3) */
    push  {r3}
    push  {r2}
    push  {r1}
    push  {r0}
    push  {r7}
    bl    arm_syscall_dispatch
    add   sp, sp, #20            /* pop 5 args */
    /* Store return value (EAX equivalent) in saved r0 on stack */
    str   r0, [sp]               /* saved r0 = return value */
    pop   {r0-r12, pc}^          /* restore and return to user mode */

/* ---- IRQ handler ---- */
    .global _irq_handler
    .extern arm_irq_dispatch
_irq_handler:
    /* Sub 4 from LR to get the return address (ARM IRQ convention) */
    sub   lr, lr, #4
    push  {r0-r12, lr}
    bl    arm_irq_dispatch
    pop   {r0-r12, pc}^          /* restore registers + CPSR, return to interrupted mode */
