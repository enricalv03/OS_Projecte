/* ============================================================================
 * arch/arm/kernel/core/arm_main.c  —  ARM C entry point
 * ============================================================================
 * Called by boot.S after the stack and BSS are initialised.
 *
 * Initialization sequence:
 *   1. Initialise UART for early console output.
 *   2. Initialise PMM (physical page allocator).
 *   3. Initialise VMM (enable MMU with identity-mapped kernel).
 *   4. Install exception vector table via VBAR.
 *   5. Call kernel_c_init() — the shared kernel initialisation.
 *   6. Enable interrupts and idle.
 * =========================================================================== */

#include "arch.h"
#include "memory/arm_pmm.h"
#include "memory/arm_vmm.h"

/* ---- PL011 UART (QEMU -M virt, UART0 at 0x09000000) ---------------------- */
#define PL011_BASE  0x09000000u
#define UARTDR      (*(volatile unsigned int *)(PL011_BASE + 0x000u))
#define UARTFR      (*(volatile unsigned int *)(PL011_BASE + 0x018u))
#define UARTIBRD    (*(volatile unsigned int *)(PL011_BASE + 0x024u))
#define UARTFBRD    (*(volatile unsigned int *)(PL011_BASE + 0x028u))
#define UARTLCR_H   (*(volatile unsigned int *)(PL011_BASE + 0x02Cu))
#define UARTCR      (*(volatile unsigned int *)(PL011_BASE + 0x030u))

#define UART_TXFF   (1u << 5)

static void uart_init(void) {
    UARTCR    = 0;
    UARTIBRD  = 13;
    UARTFBRD  = 1;
    UARTLCR_H = (3u << 5) | (1u << 4);
    UARTCR    = (1u << 0) | (1u << 8) | (1u << 9);
}

static void uart_putc(char c) {
    while (UARTFR & UART_TXFF);
    UARTDR = (unsigned int)(unsigned char)c;
}

static void uart_puts(const char *s) {
    for (; *s != '\0'; s++) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s);
    }
}

/* ---- Vector table (defined in vectors.S) ---- */
extern unsigned int arm_vector_table;

/* ---- Forward declarations ---- */
extern void kernel_c_init(void);

/* ---- ARM kernel entry ---------------------------------------------------- */

void arm_kernel_main(void) {
    uart_init();

    uart_puts("\n");
    uart_puts("*************************************\n");
    uart_puts("*   MyOS  --  ARM booting           *\n");
    uart_puts("*************************************\n");
    uart_puts("[ARM] PMM init...\n");
    arm_pmm_init();

    uart_puts("[ARM] VMM / MMU init...\n");
    arm_vmm_init();

    /* Install exception vector table via VBAR (Cortex-A8+) */
    __asm__ volatile(
        "mcr p15, 0, %0, c12, c0, 0"   /* VBAR = &arm_vector_table */
        :: "r"(&arm_vector_table) : "memory"
    );
    uart_puts("[ARM] Exception vectors installed.\n");

    /* Call the shared kernel C initialiser (VFS, scheduler, syscall table).
     * arch_timer_init() inside kernel_c_init() will call arm_irq_init() which
     * sets up the GIC and ARM Generic Timer. */
    uart_puts("[ARM] Calling kernel_c_init()...\n");
    kernel_c_init();

    uart_puts("[ARM] Kernel initialised. Enabling interrupts.\n");
    arch_enable_interrupts();

    uart_puts("[ARM] Idle loop.\n");
    for (;;) {
        arch_idle();
    }
}
