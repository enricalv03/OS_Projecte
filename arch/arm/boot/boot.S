/* ============================================================================
 * arch/arm/boot/boot.S  —  ARMv7-A entry point for QEMU -M virt
 * ============================================================================
 * QEMU loads the ELF and transfers control to _start.
 *
 * Responsibilities:
 *   1. Enter Supervisor mode with all interrupts masked.
 *   2. Set up an initial kernel stack (grows down from 0x40100000).
 *   3. Zero-initialise the BSS section.
 *   4. Transfer control to arm_kernel_main() in arm_main.c.
 *
 * Stack layout used here:
 *   0x40008000 .. 0x400FFFFF  kernel text+data+bss
 *   0x40100000                stack top (grows downward)
 * =========================================================================== */

    .section .text._start
    .global  _start
    .type    _start, %function

_start:
    /* Enter Supervisor mode (SVC) with IRQ/FIQ/abort masked.
     * CPSR[4:0] = 10011 (SVC), I/F/A bits set. */
    cpsid aif, #0x13

    /* Initialise banked exception stacks BEFORE enabling interrupts.
     * Without a valid IRQ-mode SP, the first IRQ push corrupts memory. */
    cpsid aif, #0x12            /* IRQ mode */
    ldr   sp, =0x400FE000       /* IRQ stack top */

    cpsid aif, #0x17            /* Abort mode */
    ldr   sp, =0x400FD000       /* Abort stack top */

    cpsid aif, #0x1B            /* Undefined mode */
    ldr   sp, =0x400FC000       /* Undef stack top */

    cpsid aif, #0x13            /* Back to SVC mode */

    /* Set up SVC/kernel stack */
    ldr   sp, =0x40100000

    /* Zero BSS (__bss_start / __bss_end are defined by linker.ld) */
    ldr   r0, =__bss_start
    ldr   r1, =__bss_end
    mov   r2, #0
.Lbss_loop:
    cmp   r0, r1
    bge   .Lbss_done
    str   r2, [r0], #4
    b     .Lbss_loop
.Lbss_done:

    /* Jump to C entry point — must not return */
    bl    arm_kernel_main

    /* Defensive spin if arm_kernel_main ever returns */
.Lhang:
    wfi
    b     .Lhang
