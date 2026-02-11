#include "kernel.h"
#include "gdt.h"
#include "../memory/heap.h"
#include "../process/process.h"
#include "../process/scheduler.h"
#include "../syscall/syscall.h"
#include "../drivers/block.h"
#include "../fs/diskfs.h"

#define USER_STACK_TOP 0x80000

/* Defined in pit.asm -- arms the PIT handler to call scheduler_tick() */
extern void pit_enable_scheduler(void);

extern void enter_user_mode(unsigned int user_eip, unsigned int user_esp);

/* Simple test function to run in ring 3.
 * Writes directly to VGA text memory so we don't depend on syscalls.
 * Enable the test block at the end of kernel_c_init when you want to run it. */
static void user_demo(void) {
    /* Write on a separate line so we don't overwrite the kernel banner.
     * Each row is 160 bytes (80 cols * 2 bytes per character). */
    volatile unsigned char* vga = (unsigned char*)(0xB8000 + 160 * 5); /* row 5 */
    const char msg[] = "USER MODE OK";

    for (int i = 0; msg[i] != 0; i++) {
        vga[i * 2]     = (unsigned char)msg[i];  /* character */
        vga[i * 2 + 1] = 0x0F;                   /* white on black */
    }

    /* Stay alive in user mode so you can attach a debugger later if needed. */
    for (;;) {
        asm volatile ("hlt");
    }
}
/* =============================================================================
 * kernel_c_init()
 * =============================================================================
 * Called from kernel.asm BEFORE sti (interrupts still disabled).
 *
 * Initializes all C subsystems in dependency order:
 *   1. Heap      -- dynamic allocation (malloc/free)
 *   2. Scheduler -- scheduling queues (MLFQ + CFS)
 *   3. Process   -- kernel process (PID 0)
 *   4. Syscalls  -- INT 0x80 handler table
 *   5. Block I/O -- ATA PIO driver
 *   6. Enable scheduler ticking in PIT handler
 *
 * After this returns, kernel.asm enables interrupts (sti) and enters
 * the console loop. The PIT will now safely call scheduler_tick().
 * =========================================================================== */
void kernel_c_init(void) {
    /* Step 0: Set up the full GDT with user-mode segments and TSS.
     * This replaces the minimal 3-entry GDT from stage2.asm with
     * 6 entries: null, kernel CS/DS, user CS/DS, and TSS.
     * Must be done first since everything else depends on valid segments. */
    gdt_init();

    /* Step 1: Initialize the kernel heap allocator
     * Maps virtual pages at 0x10000000 and sets up the free list.
     * After this, malloc() and free() are available. */
    heap_init();

    /* Step 2: Initialize the scheduler (MLFQ + CFS)
     * Clears all scheduling queues. Must happen before process_init()
     * because process creation enqueues into the scheduler. */
    scheduler_init();

    /* Step 3: Initialize the process subsystem
     * Creates the kernel process (PID 0) with REALTIME priority.
     * Sets it as the current running process. */
    process_init();

    /* Tell the scheduler about the kernel process */
    scheduler_set_current(process_get_current());

    /* Step 4: Initialize the system call interface
     * Registers handlers for exit, fork, exec, getpid, etc.
     * Note: The INT 0x80 IDT entry is set up separately in ASM. */
    syscall_init();

    /* Step 5: Initialize block device subsystem and ATA driver
     * Probes ATA primary master, registers it as a block device.
     * This enables disk read/write through the block layer. */
    block_init();

    /* Step 6: Probe for SimpleFS on disk and register any files.
     * This reads sectors 200+ via the ATA driver and adds files
     * to the RAMFS root so they appear in ls/cat.
     * It's fine if this fails (no filesystem on disk). */
    diskfs_init();

    /* Step 7: NOW it's safe to let the PIT call scheduler_tick().
     * This sets the sched_enabled flag in pit.asm.
     * Before this point, PIT ticks only increment the counter. */
    pit_enable_scheduler();

    /* Optional: simple Ring-3 test.
     * If you enable this block, the kernel will jump into user_demo()
     * in ring 3 and never return to the shell. Leave it disabled by
     * default so you can use the shell. */
#if 0
    tss_set_kernel_stack(0x90000); /* kernel stack top used on interrupts */
    enter_user_mode((unsigned int)user_demo, USER_STACK_TOP);
#endif
}
