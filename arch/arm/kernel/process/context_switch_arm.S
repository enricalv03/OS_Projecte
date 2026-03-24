/* ============================================================================
 * arch/arm/kernel/process/context_switch_arm.S — ARMv7 Context Switch
 * ============================================================================
 * Saves the current process's registers into its PCB and restores the next
 * process's registers from its PCB.
 *
 * C prototype (matches kernel/sched/process.h, architecture-specific fields):
 *   void context_switch_asm(pcb_t* from, pcb_t* to);
 *
 * ARM register save offsets inside pcb_t (must match process.h):
 *   We save r0-r12 and SP/LR/PC/CPSR.  The pcb_t structure stores these as:
 *     eax=r0  ebx=r1  ecx=r2  edx=r3
 *     esi=r4  edi=r5  ebp=r6  esp=sp
 *     eip=pc
 *     eflags = cpsr
 *   (Other fields: r7-r12 are not individually stored in pcb_t currently;
 *    this is a minimal save of the callee-saved and special registers.)
 *
 * NOTE: A full implementation would also save/restore the FPU state and
 * handle the SPSR correctly.  This minimal version is sufficient for
 * cooperative context switches between C kernel threads.
 * =========================================================================== */

    .section .text
    .global context_switch_asm

/* context_switch_asm(pcb_t* from, pcb_t* to)
 * r0 = from, r1 = to  (ARM calling convention) */
context_switch_asm:
    /* Save callee-saved registers + SP + LR of the current process.
     * We push {r4-r11, lr} onto the current stack and record the new SP. */
    push  {r4-r11, lr}

    /* If from is NULL, skip saving */
    cmp   r0, #0
    beq   .Lno_save

    /* Store SP into from->esp (offset 28 in pcb_t — see process.h field order).
     * The pcb_t layout (fields in declaration order, 4 bytes each):
     *   +0   pid
     *   +4   parent_pid
     *   +8   state
     *   +12  priority
     *   +16  base_priority
     *   +20  eax (r0 saved value)
     *   +24  ebx
     *   +28  ecx
     *   +32  edx
     *   +36  esi
     *   +40  edi
     *   +44  ebp
     *   +48  esp  ← this is where we store the current SP
     *   +52  eip
     *   +56  eflags
     * We use a calculated offset.  Since pcb_t is declared with fixed-size
     * unsigned int fields, we can count them manually.
     * pid(0)+parent_pid(4)+state(8)+priority(12)+base_priority(16) = 5 fields
     * eax(20)+ebx(24)+ecx(28)+edx(32)+esi(36)+edi(40)+ebp(44) = 7 more fields
     * esp is at byte offset 48 from the start of pcb_t.
     */
    str   sp, [r0, #48]      /* from->esp = sp */
    /* Save LR as EIP (the point we'll resume from) */
    str   lr, [r0, #52]      /* from->eip = lr */
    /* Save CPSR as eflags */
    mrs   r2, cpsr
    str   r2, [r0, #56]      /* from->eflags = cpsr */

.Lno_save:
    /* Restore the 'to' process */
    cmp   r1, #0
    beq   .Lhalt

    /* Load to->esp */
    ldr   sp, [r1, #48]
    /* Load to->eip as LR (we'll branch to it via pop {pc}) */
    ldr   lr, [r1, #52]
    /* Restore to->eflags (CPSR) */
    ldr   r2, [r1, #56]
    msr   cpsr_c, r2

    /* Restore callee-saved registers from the new stack */
    pop   {r4-r11, pc}       /* pc = saved lr from push above = resume point */

.Lhalt:
    b     .Lhalt
