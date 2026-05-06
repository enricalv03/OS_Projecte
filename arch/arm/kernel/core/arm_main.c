/* ============================================================================
 * arch/arm/kernel/core/arm_main.c  —  ARM C entry point
 * ============================================================================ */

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
     while (UARTFR & UART_TXFF) { }
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
 extern void arm_irq_init(unsigned int ticks_per_second);
extern void arm_set_tick_hook(void (*hook)(void));
extern void scheduler_tick(void);

static volatile unsigned int arm_local_tick_count = 0;

static void arm_tick_callback(void) {
    arm_local_tick_count++;
    scheduler_tick();
}
 
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
 
     /* Install exception vectors via VBAR + barriers */
     __asm__ volatile(
         "mcr p15, 0, %0, c12, c0, 0 \n"
         "dsb                          \n"
         "isb                          \n"
         :
         : "r"(&arm_vector_table)
         : "memory"
     );
     uart_puts("[ARM] Exception vectors installed.\n");
 
     uart_puts("[ARM] Initialising IRQ/timer (100 Hz)...\n");
     arm_irq_init(100);
    arm_set_tick_hook(arm_tick_callback);
 
     uart_puts("[ARM] Enabling interrupts...\n");
     arch_enable_interrupts();
 
     /* Diagnostic: trigger SGI to self (CPU0).
        If IRQ path works, arm_irq_dispatch prints I? (or similar). */
     /*uart_puts("[ARM] Trigger SGI self-test...\n");
     arch_send_ipi(0);*/
 
     uart_puts("[ARM] Idle loop.\n");
     for (;;) {
         arch_idle();
     }
 }