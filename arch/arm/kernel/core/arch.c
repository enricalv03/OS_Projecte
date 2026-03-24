/* ============================================================================
 * arch/arm/kernel/core/arch.c  —  ARMv7-A implementation of kernel/arch.h
 * ============================================================================
 * Provides the arch-neutral functions declared in kernel/arch.h using ARMv7
 * instructions and the ARM Generic Timer (CNTV / CNTP) for timing.
 *
 * Timer implementation note:
 *   The ARM Generic Timer is available on Cortex-A15 and above (QEMU -M virt
 *   with -cpu cortex-a15).  For simplicity we use a software counter incremented
 *   by the FIQ/IRQ handler stubs; a real implementation would configure the
 *   GIC (Generic Interrupt Controller) and program CNTP_CTL_EL0.
 * =========================================================================== */

#include "arch.h"

/* ---- CPU control ---------------------------------------------------------- */

void arch_enable_interrupts(void) {
    __asm__ volatile("cpsie i" ::: "memory");
}

void arch_disable_interrupts(void) {
    __asm__ volatile("cpsid i" ::: "memory");
}

void arch_halt(void) {
    /* WFI: suspend execution until an interrupt or event wakes the core. */
    __asm__ volatile("wfi" ::: "memory");
}

void arch_idle(void) {
    /* Re-enable IRQ then wait; the IRQ handler will wake us. */
    __asm__ volatile("cpsie i" ::: "memory");
    __asm__ volatile("wfi"    ::: "memory");
}

/* ---- Timer ---------------------------------------------------------------- */

/* Software tick counter — bumped by the timer interrupt handler.
 * Replace with a read of the ARM Generic Timer physical counter when the
 * GIC + timer interrupt are fully wired up. */
static volatile unsigned int arm_ticks = 0;

/* Called from the future timer ISR (not yet hooked to a real interrupt). */
void arm_timer_tick(void) {
    arm_ticks++;
}

/* Forward declaration — implemented in arm_irq.c */
extern void arm_irq_init(unsigned int ticks_per_second);

void arch_timer_init(unsigned int ticks_per_second) {
    arm_irq_init(ticks_per_second);
}

unsigned int arch_timer_get_ticks(void) {
    return arm_ticks;
}

/* ---- SMP ------------------------------------------------------------------ */

void arch_send_ipi(unsigned int dest_node_id) {
    /* On ARM, inter-processor interrupts use the GIC Software Generated
     * Interrupt register (GICD_SGIR).  Writing to it sends SGI #0 to
     * the target CPU interface identified by dest_node_id.
     * This is a placeholder; adjust the GIC base and interface numbering
     * to match the actual platform when real SMP support is needed. */
    #define GIC_DIST_BASE  0x08000000u
    #define GICD_SGIR      (*(volatile unsigned int *)(GIC_DIST_BASE + 0xF00u))
    /* Bits [25:24] = 0b01 (target list), bits [19:16] = cpu interface mask,
     * bits [3:0] = SGI ID 0. */
    GICD_SGIR = (1u << 24) | ((1u << dest_node_id) << 16) | 0u;
}
