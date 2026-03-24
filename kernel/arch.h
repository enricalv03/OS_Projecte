#ifndef KERNEL_ARCH_H
#define KERNEL_ARCH_H

/* =============================================================================
 * Generic arch interface
 * =============================================================================
 * All functions declared here are implemented per-architecture under
 * arch/<arch>/kernel/core/arch.c.  Generic kernel code (scheduler, syscalls,
 * panic, etc.) must use only these functions — never raw `sti`, `cli`, `hlt`,
 * or hardware-specific I/O ports directly.
 *
 * x86 implementation:  arch/x86/kernel/core/arch.c
 * (future ARM impl):   arch/arm/kernel/core/arch.c
 * =========================================================================== */

/* ---- CPU control ---------------------------------------------------------- */

void arch_enable_interrupts(void);
void arch_disable_interrupts(void);
void arch_halt(void);      /* halt the CPU until next interrupt               */
void arch_idle(void);      /* safe idle loop: enable interrupts then halt      */

static inline void arch_pause(void) {
    __asm__ __volatile__("nop");
}

/* ---- Timer ---------------------------------------------------------------- */

/* arch_timer_init -- initialise the platform timer at the requested rate and
 * arm the scheduler-tick callback.  Call once during kernel init.
 * ticks_per_second: desired tick frequency (e.g. 100 for 100 Hz). */
void arch_timer_init(unsigned int ticks_per_second);

/* arch_timer_get_ticks -- return the raw tick counter since boot. */
unsigned int arch_timer_get_ticks(void);

/* ---- SMP ------------------------------------------------------------------ */

/* arch_send_ipi -- send a scheduler-wake IPI to the CPU with the given
 * node/LAPIC id.  On single-core builds or non-SMP arches this is a no-op. */
void arch_send_ipi(unsigned int dest_node_id);

#endif
