/* ============================================================================
 * arch/arm/kernel/core/arm_main.c  —  ARM C entry point
 * ============================================================================ */

 #include "arch.h"
#include "sched/process.h"
#include "sched/scheduler.h"
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
extern void process_init(void);
extern void arm_timer_tick(void);
extern void process_sleep_ticks(unsigned int ticks);

static volatile unsigned int arm_local_tick_count = 0;
static volatile unsigned int arm_demo_print_div = 0;
static volatile unsigned int arm_shortlived_count = 0;
static volatile unsigned int arm_last_spawn_tick = 0;

static void arm_sleeper_thread(void) {
    uart_puts("[ARM] sleeper thread started\n");
    for (;;) {
        uart_puts("[ARM] sleeper: going to sleep (50 ticks)\n");
        process_sleep_ticks(50u);
        uart_puts("[ARM] sleeper: woke up\n");
    }
}

static void arm_shortlived_thread(void) {
    uart_puts("[ARM] short thread start+exit\n");
    arm_shortlived_count++;
    process_exit(0);
}

static void arm_demo_thread(void) {
    uart_puts("[ARM] demo thread started\n");
    for (;;) {
        if (arm_local_tick_count != 0 &&
            arm_local_tick_count != arm_last_spawn_tick &&
            (arm_local_tick_count % 200u) == 0u) {
            arm_last_spawn_tick = arm_local_tick_count;
            if (process_create_kernel_thread("arm_short", PRIORITY_HIGH, arm_shortlived_thread)) {
                uart_puts("[ARM] short thread spawned\n");
            } else {
                uart_puts("[ARM] short thread spawn failed\n");
            }
        }

        /* Print occasionally so we can observe non-idle execution. */
        if ((arm_demo_print_div++ % 500000u) == 0u) {
            uart_puts("[ARM] demo thread tick\n");
        }
        scheduler_yield();
    }
}

static void arm_tick_callback(void) {
    arm_local_tick_count++;
    arm_timer_tick();
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
    process_init();
 
     uart_puts("[ARM] Enabling interrupts...\n");
     arch_enable_interrupts();

    if (process_create_kernel_thread("arm_demo", PRIORITY_NORMAL, arm_demo_thread)) {
        uart_puts("[ARM] demo thread created\n");
        scheduler_yield();
    } else {
        uart_puts("[ARM] demo thread create failed\n");
    }

    if (process_create_kernel_thread("arm_sleep", PRIORITY_LOW, arm_sleeper_thread)) {
        uart_puts("[ARM] sleeper thread created\n");
    } else {
        uart_puts("[ARM] sleeper thread create failed\n");
    }
 
     /* Diagnostic: trigger SGI to self (CPU0).
        If IRQ path works, arm_irq_dispatch prints I? (or similar). */
     /*uart_puts("[ARM] Trigger SGI self-test...\n");
     arch_send_ipi(0);*/
 
     uart_puts("[ARM] Idle loop.\n");
     for (;;) {
        scheduler_yield();
         arch_idle();
     }
 }