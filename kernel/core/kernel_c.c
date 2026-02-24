#include "kernel.h"
#include "gdt.h"
#include "../memory/heap.h"
#include "../process/process.h"
#include "../process/scheduler.h"
#include "../syscall/syscall.h"
#include "../drivers/block.h"
#include "../fs/diskfs.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"

#define USER_STACK_TOP 0x80000

/* -------------------------------------------------------------------------
 * Minimal user-mode syscall wrappers
 * -------------------------------------------------------------------------
 * These helpers are compiled into the kernel binary but are intended to be
 * executed from ring 3 after enter_user_mode(). They invoke the existing
 * INT 0x80 syscall interface so our first user-mode code can do basic I/O.
 * ------------------------------------------------------------------------- */
static int u_sys_write(unsigned int fd, const char* buf, unsigned int count) {
    int ret;
    /* Use fixed register constraints so GCC knows exactly
     * which registers we are setting up for INT 0x80. */
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

static int u_sys_read(unsigned int fd, char* buf, unsigned int count) {
    int ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

/* First real user-mode test program.
 * Runs in ring 3. It writes once directly to VGA (to prove that the
 * ring-3 transition works), then becomes a tiny user-mode echo loop
 * using sys_read/sys_write. */
static void user_init(void) {
    /* Direct VGA write at row 6 so we don't overlap the kernel banner.
     * This does NOT depend on syscalls at all. */
    volatile unsigned char* vga = (unsigned char*)(0xB8000 + 160 * 6);
    const char direct_msg[] = "USER MODE (direct VGA)";
    for (int i = 0; direct_msg[i] != 0; i++) {
        vga[i * 2]     = (unsigned char)direct_msg[i];
        vga[i * 2 + 1] = 0x0F;  /* white on black */
    }

    /* Print a simple prompt using sys_write (fd=1 = stdout). */
    const char prompt[] = "\n[u-shell] Type keys, they will echo back:\n";
    u_sys_write(1, prompt, sizeof(prompt) - 1);

    /* Tiny user-mode echo loop:
     * - sys_read(0, &c, 1) pulls from the keyboard buffer (fd=0 = stdin)
     * - sys_write(1, &c, 1) writes it back to the console. */
    char c;
    for (;;) {
        int n = u_sys_read(0, &c, 1);
        if (n > 0) {
            u_sys_write(1, &c, 1);
        } else {
            /* Nothing available: just spin for now.
             * Later we can call sys_yield/sys_sleep to avoid busy waiting. */
            asm volatile ("nop");
        }
    }
}

/* Defined in pit.asm -- arms the PIT handler to call scheduler_tick() */
extern void pit_enable_scheduler(void);

extern void enter_user_mode(unsigned int user_eip, unsigned int user_esp);
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
    gdt_init();

    vfs_mount_root(ramfs_get_vfs_root());

    heap_init();

    scheduler_init();

    process_init();

    scheduler_set_current(process_get_current());

    syscall_init();

    /* TEMP DEBUG: skip block_init + diskfs_init to isolate crash */
    /* block_init(); */
    /* diskfs_init(); */

    {
        vfs_node_t* home = vfs_lookup("/home/normal");
        unsigned int addr = (unsigned int)home;
        if (home && addr >= 0x3000 && addr < 0x400000 && home->type == VFS_NODE_DIR) {
            vfs_set_cwd_with_path(home, "/home/normal");
        } else {
            vfs_set_cwd(vfs_get_root());
        }
    }

    /* Step 7: NOW it's safe to let the PIT call scheduler_tick().
     * This sets the sched_enabled flag in pit.asm.
     * Before this point, PIT ticks only increment the counter. */
    pit_enable_scheduler();

    /* Optional: simple Ring-3 test.
     * With this block enabled, the kernel will jump into user_init() in
     * ring 3 and never return to the kernel shell. This is our first
     * real user-mode program using the syscall path. */
#if 0
    tss_set_kernel_stack(0x90000); /* kernel stack top used on interrupts */
    enter_user_mode((unsigned int)user_init, USER_STACK_TOP);
#endif
}
