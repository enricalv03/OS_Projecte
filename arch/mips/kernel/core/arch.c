/* MIPS arch.h implementation stub */
#include "arch.h"

void arch_enable_interrupts(void) {
    /* TODO: set IE bit in CP0 Status register */
}
void arch_disable_interrupts(void) {
    /* TODO: clear IE bit in CP0 Status register */
}
void arch_halt(void) {
    __asm__ volatile("wait" ::: "memory");
}
void arch_idle(void) { arch_enable_interrupts(); arch_halt(); }
void arch_timer_init(unsigned int ticks_per_second) { (void)ticks_per_second; }
unsigned int arch_timer_get_ticks(void) { return 0; }
