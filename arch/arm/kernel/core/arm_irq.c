/* ============================================================================
 * arch/arm/kernel/core/arm_irq.c — GIC init and IRQ dispatcher for ARMv7
 * ============================================================================ */

 #include "arch.h"

 /* ---- GIC register blocks ---- */
 #define GICD_BASE   0x08000000u
 #define GICC_BASE   0x08010000u
 
 /* GIC Distributor registers */
 #define GICD_CTLR         (*(volatile unsigned int *)(GICD_BASE + 0x000u))
 #define GICD_TYPER        (*(volatile unsigned int *)(GICD_BASE + 0x004u))
 #define GICD_ISENABLER(n) (*(volatile unsigned int *)(GICD_BASE + 0x100u + (n) * 4u))
 #define GICD_IPRIORITYR(n) (*(volatile unsigned int *)(GICD_BASE + 0x400u + (n) * 4u))
 #define GICD_ITARGETSR(n)  (*(volatile unsigned int *)(GICD_BASE + 0x800u + (n) * 4u))
 
 /* GIC CPU Interface registers */
 #define GICC_CTLR    (*(volatile unsigned int *)(GICC_BASE + 0x000u))
 #define GICC_PMR     (*(volatile unsigned int *)(GICC_BASE + 0x004u))
 #define GICC_IAR     (*(volatile unsigned int *)(GICC_BASE + 0x00Cu))
 #define GICC_EOIR    (*(volatile unsigned int *)(GICC_BASE + 0x010u))
 
 /* ARM Generic Timer (CP15 c14) */
 static inline unsigned int cntfrq_read(void) {
     unsigned int v;
     __asm__ __volatile__("mrc p15, 0, %0, c14, c0, 0" : "=r"(v));
     return v;
 }
 
 static inline void cntp_tval_set(unsigned int v) {
     __asm__ __volatile__("mcr p15, 0, %0, c14, c2, 0" : : "r"(v) : "memory");
 }
 
 static inline void cntp_ctl_set(unsigned int v) {
     __asm__ __volatile__("mcr p15, 0, %0, c14, c2, 1" : : "r"(v) : "memory");
 }
 
 /* ---- PL011 UART (debug output) ---- */
 #define PL011_BASE  0x09000000u
 #define UARTDR      (*(volatile unsigned int *)(PL011_BASE + 0x000u))
 #define UARTFR      (*(volatile unsigned int *)(PL011_BASE + 0x018u))
 #define UART_TXFF   (1u << 5)
 
 static void uart_putc(char c) {
     while (UARTFR & UART_TXFF) { }
     UARTDR = (unsigned int)(unsigned char)c;
 }
 
 /* Software tick counter */
 static volatile unsigned int arm_ticks_gic = 0;
 static unsigned int timer_reload = 0;
static void (*arm_tick_hook)(void) = 0;
 
 /* External C function called every timer tick (defined in arch.c) */
 void arm_timer_tick(void);
 
 /* ---- GIC Initialization ---- */
 static void gic_init(void) {
     /* Disable distributor first */
     GICD_CTLR = 0;
 
     unsigned int num_irqs = ((GICD_TYPER & 0x1Fu) + 1u) * 32u;
 
     /* Priorities + targets for SPIs */
     for (unsigned int i = 32; i < num_irqs; i += 4) {
         GICD_IPRIORITYR(i / 4) = 0xA0A0A0A0u;
         GICD_ITARGETSR(i / 4)  = 0x01010101u; /* CPU0 */
     }
 
     /* Priorities for SGI/PPI */
     for (unsigned int i = 0; i < 32; i += 4) {
         GICD_IPRIORITYR(i / 4) = 0xA0A0A0A0u;
     }
 
     /* Enable PPI 30 (CNTP) */
     GICD_ISENABLER(0) = (1u << 30);
 
     /* Enable both groups to avoid secure/non-secure trap differences */
     GICD_CTLR = 0x3u;
 
     /* CPU interface: accept all priorities, enable both groups */
     GICC_PMR  = 0xFFu;
     GICC_CTLR = 0x3u;
 }
 
 /* ---- Timer initialization ---- */
 static void timer_init(unsigned int ticks_per_second) {
     unsigned int freq = cntfrq_read();
     if (freq == 0) freq = 24000000u;
     if (ticks_per_second == 0) ticks_per_second = 100u;
 
     timer_reload = freq / ticks_per_second;
     if (timer_reload == 0) timer_reload = 1u;
 
     cntp_tval_set(timer_reload);
     cntp_ctl_set(1u); /* enable=1, imask=0 */
 }
 
 /* ---- Public init ---- */
 void arm_irq_init(unsigned int ticks_per_second) {
     gic_init();
     timer_init(ticks_per_second);
 }

void arm_set_tick_hook(void (*hook)(void)) {
    arm_tick_hook = hook;
}
 
 /* ---- IRQ dispatcher ---- */
 void arm_irq_dispatch(void) {
    unsigned int iar = GICC_IAR;
    unsigned int irq = iar & 0x3FFu;

    if (irq == 30u) {
        cntp_tval_set(timer_reload);
        arm_ticks_gic++;
        arm_timer_tick();
        if (arm_tick_hook) {
            arm_tick_hook();
        }

        /* Clean heartbeat: one dot every ~1 second at 100Hz */
        if ((arm_ticks_gic % 100u) == 0u) {
            uart_putc('.');
        }
    }

    GICC_EOIR = iar;
}
 
 /* ---- Fault handler ---- */
 void arm_fault_handler(unsigned int type) {
     (void)type;
     arch_disable_interrupts();
     for (;;) arch_halt();
 }
 
 /* ---- SVC dispatcher ---- */
 int arm_syscall_dispatch(unsigned int num, unsigned int a1,
                          unsigned int a2, unsigned int a3,
                          unsigned int a4) {
     (void)num; (void)a1; (void)a2; (void)a3; (void)a4;
     return -1;
 }