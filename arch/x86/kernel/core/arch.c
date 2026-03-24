/* =============================================================================
 * x86 arch interface implementation
 * =============================================================================
 * Provides the arch-neutral functions declared in kernel/arch.h using x86
 * assembly helpers (sti/cli/hlt) and the Programmable Interval Timer (PIT).
 * =========================================================================== */

#include "arch.h"
#include "core/smp.h"

/* ---- assembly stubs defined in kernel.asm / pit.asm ---------------------- */
extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void halt_cpu(void);
extern void idle_cpu(void);
extern void pit_enable_scheduler(void);
extern unsigned int pit_get_ticks(void);

/* ---- CPU control ---------------------------------------------------------- */

void arch_enable_interrupts(void) {
    enable_interrupts();
}

void arch_disable_interrupts(void) {
    disable_interrupts();
}

void arch_halt(void) {
    halt_cpu();
}

void arch_idle(void) {
    idle_cpu();
}

/* ---- Timer ---------------------------------------------------------------- */

/* arch_timer_init -- The PIT divisor is programmed once at boot by kernel.asm
 * to run at ~100 Hz.  ticks_per_second is accepted for documentation purposes
 * but is not used to reprogram the PIT here; future enhancement can add that.
 * This call arms the scheduler-tick pathway inside the PIT ISR. */
void arch_timer_init(unsigned int ticks_per_second) {
    (void)ticks_per_second;
    pit_enable_scheduler();
}

unsigned int arch_timer_get_ticks(void) {
    return pit_get_ticks();
}

/* ---- SMP ------------------------------------------------------------------ */

void arch_send_ipi(unsigned int dest_node_id) {
    lapic_send_ipi(dest_node_id, IPI_VECTOR_SCHED_WAKE);
}
