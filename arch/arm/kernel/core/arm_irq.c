/* ============================================================================
 * arch/arm/kernel/core/arm_irq.c — GIC init and IRQ dispatcher for ARMv7
 * ============================================================================
 * Targets the ARM GIC-400 (PL390) present in QEMU -M virt.
 *
 * The GIC on QEMU virt:
 *   Distributor  @ 0x08000000
 *   CPU interface @ 0x08010000
 *
 * Timer PPI:
 *   IRQ 30 (CNTP_EL0, physical non-secure EL1)
 *   Delivered as PPI on the local CPU interface.
 * =========================================================================== */

#include "arch.h"

/* ---- GIC register blocks ---- */
#define GICD_BASE   0x08000000u
#define GICC_BASE   0x08010000u

/* GIC Distributor registers */
#define GICD_CTLR      (*(volatile unsigned int *)(GICD_BASE + 0x000u))
#define GICD_TYPER     (*(volatile unsigned int *)(GICD_BASE + 0x004u))
#define GICD_ISENABLER(n) (*(volatile unsigned int *)(GICD_BASE + 0x100u + (n)*4u))
#define GICD_IPRIORITYR(n) (*(volatile unsigned int *)(GICD_BASE + 0x400u + (n)*4u))
#define GICD_ITARGETSR(n)  (*(volatile unsigned int *)(GICD_BASE + 0x800u + (n)*4u))
#define GICD_ICFGR(n)      (*(volatile unsigned int *)(GICD_BASE + 0xC00u + (n)*4u))

/* GIC CPU Interface registers */
#define GICC_CTLR    (*(volatile unsigned int *)(GICC_BASE + 0x000u))
#define GICC_PMR     (*(volatile unsigned int *)(GICC_BASE + 0x004u))
#define GICC_IAR     (*(volatile unsigned int *)(GICC_BASE + 0x00Cu))
#define GICC_EOIR    (*(volatile unsigned int *)(GICC_BASE + 0x010u))

/* ARM Generic Timer (CP15 c14) */
#define CNTFRQ()  ({ unsigned int _v; __asm__("mrc p15,0,%0,c14,c0,0":"=r"(_v)); _v; })
#define CNTP_TVAL_SET(v) __asm__ volatile("mcr p15,0,%0,c14,c2,0"::"r"(v):"memory")
#define CNTP_CTL_SET(v)  __asm__ volatile("mcr p15,0,%0,c14,c2,1"::"r"(v):"memory")

/* Software tick counter */
static volatile unsigned int arm_ticks_gic = 0;
static unsigned int timer_reload = 0;

/* External C function called every timer tick (defined in arch.c) */
void arm_timer_tick(void);

/* ---- GIC Initialization ---- */
static void gic_init(void) {
    /* Disable distributor */
    GICD_CTLR = 0;

    /* Set all SPIs to CPU0, priority 0xA0, level-triggered */
    unsigned int num_irqs = ((GICD_TYPER & 0x1Fu) + 1u) * 32u;
    for (unsigned int i = 32; i < num_irqs; i += 4) {
        GICD_IPRIORITYR(i / 4) = 0xA0A0A0A0u;
        GICD_ITARGETSR(i / 4)  = 0x01010101u;  /* CPU 0 */
    }

    /* Also set PPI/SGI priorities (0-31) */
    for (unsigned int i = 0; i < 32; i += 4) {
        GICD_IPRIORITYR(i / 4) = 0xA0A0A0A0u;
    }

    /* Enable IRQ 30 (CNTP timer PPI) */
    GICD_ISENABLER(0) = (1u << 30);   /* PPI 30 is in the first ISENABLER */

    /* Enable distributor */
    GICD_CTLR = 1;

    /* CPU interface: allow all priorities, enable */
    GICC_PMR  = 0xF0;
    GICC_CTLR = 1;
}

/* ---- Timer initialization ---- */
static void timer_init(unsigned int ticks_per_second) {
    unsigned int freq = CNTFRQ();
    if (freq == 0) freq = 24000000u;   /* QEMU default: 24 MHz */
    timer_reload = freq / ticks_per_second;

    /* Program the timer: set TVAL and enable */
    CNTP_TVAL_SET(timer_reload);
    CNTP_CTL_SET(1u);   /* bit 0 = enable, bit 1 = IMASK (0 = unmask) */
}

/* ---- Public init ---- */
void arm_irq_init(unsigned int ticks_per_second) {
    gic_init();
    timer_init(ticks_per_second);
}

/* ---- IRQ dispatcher (called from vectors.S) ---- */
void arm_irq_dispatch(void) {
    unsigned int iar = GICC_IAR;
    unsigned int irq = iar & 0x3FFu;

    if (irq == 30) {
        /* Timer PPI: acknowledge, reload, and tick */
        CNTP_TVAL_SET(timer_reload);
        arm_ticks_gic++;
        arm_timer_tick();   /* calls scheduler_tick on ARM eventually */
    }

    /* End-of-interrupt */
    GICC_EOIR = iar;
}

/* ---- Fault handler (called from vectors.S) ---- */
void arm_fault_handler(unsigned int type) {
    /* Read IFSR / DFSR for diagnostic */
    unsigned int dfsr, dfar, ifsr, ifar;
    __asm__ volatile("mrc p15,0,%0,c5,c0,0":"=r"(dfsr));
    __asm__ volatile("mrc p15,0,%0,c6,c0,0":"=r"(dfar));
    __asm__ volatile("mrc p15,0,%0,c5,c0,1":"=r"(ifsr));
    __asm__ volatile("mrc p15,0,%0,c6,c0,2":"=r"(ifar));

    (void)type; (void)dfsr; (void)dfar; (void)ifsr; (void)ifar;

    /* Print via UART would be ideal here; for now just halt */
    arch_disable_interrupts();
    for (;;) arch_halt();
}

/* ---- SVC dispatcher (called from _svc_handler in vectors.S) ---- */
int arm_syscall_dispatch(unsigned int num, unsigned int a1,
                         unsigned int a2, unsigned int a3,
                         unsigned int a4) {
    /* Delegate to the platform-neutral syscall_handle() */
    extern int syscall_handle(unsigned int, unsigned int, unsigned int,
                               unsigned int, unsigned int);
    return syscall_handle(num, a1, a2, a3, a4);
}
