/* ============================================================================
 * arch/riscv/kernel/core/arch.c — RISC-V implementation of kernel/arch.h
 * ============================================================================
 * Placeholder — only the function stubs are provided.
 * Full implementation requires:
 *   - CLINT (Core-Local Interruptor) for machine-mode timer (mtime/mtimecmp)
 *   - PLIC (Platform-Level Interrupt Controller) for external interrupts
 *   - mstatus / mie / mip CSR manipulation for interrupt enable/disable
 * ============================================================================ */

#include "arch.h"

void arch_enable_interrupts(void) {
    /* Set MIE (machine interrupt enable) in mstatus */
    __asm__ volatile("csrsi mstatus, 8" ::: "memory");
}

void arch_disable_interrupts(void) {
    /* Clear MIE in mstatus */
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
}

void arch_halt(void) {
    __asm__ volatile("wfi" ::: "memory");
}

void arch_idle(void) {
    arch_enable_interrupts();
    arch_halt();
}

static volatile unsigned int riscv_ticks = 0;

void arch_timer_init(unsigned int ticks_per_second) {
    (void)ticks_per_second;
    /* TODO: program CLINT mtimecmp for timer interrupts at ticks_per_second Hz */
}

unsigned int arch_timer_get_ticks(void) {
    return riscv_ticks;
}
