/* ============================================================================
 * arch/arm/kernel/core/arch.c  —  ARMv7-A implementation of kernel/arch.h
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
     __asm__ volatile("wfi" ::: "memory");
 }
 
 void arch_idle(void) {
     __asm__ volatile("cpsie i" ::: "memory");
     __asm__ volatile("wfi" ::: "memory");
 }
 
 /* ---- Timer ---------------------------------------------------------------- */
 
 static volatile unsigned int arm_ticks = 0;
 
 void arm_timer_tick(void) {
     arm_ticks++;
 }
 
 extern void arm_irq_init(unsigned int ticks_per_second);
 
 void arch_timer_init(unsigned int ticks_per_second) {
     arm_irq_init(ticks_per_second);
 }
 
 unsigned int arch_timer_get_ticks(void) {
     return arm_ticks;
 }
 
 /* ---- SMP / IPI ------------------------------------------------------------ */
 
 void arch_send_ipi(unsigned int dest_node_id) {
     (void)dest_node_id;
 
     /* GICv2 Distributor SGIR register on QEMU virt */
     #define GIC_DIST_BASE  0x08000000u
     #define GICD_SGIR      (*(volatile unsigned int *)(GIC_DIST_BASE + 0xF00u))
 
     /* Send SGI ID 0 to SELF:
        TargetListFilter bits [25:24] = 0b10 (self)
        CPUTargetList ignored in this mode.
     */
     GICD_SGIR = (2u << 24) | 0u;
 }