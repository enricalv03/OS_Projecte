#include "kernel.h"
#include "gdt.h"
#include "node.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "sched/process.h"
#include "sched/scheduler.h"
#include "sys/syscall.h"
#include "drivers/block.h"
#include "fs/diskfs.h"
#include "fs/vfs.h"
#include "fs/ramfs.h"
#include "fs/devfs.h"
/* Networking: virtio-net driver.  Temporarily optional while we stabilise
 * SMP + paging; enable when running under QEMU with virtio-net attached. */
/* #include "net/virtio_net.h" */

#define USER_STACK_TOP 0x80000

/* -------------------------------------------------------------------------
 * Minimal user-mode syscall wrappers
 * -------------------------------------------------------------------------
 * These helpers are compiled into the kernel binary but are intended to be
 * executed from ring 3 after enter_user_mode(). They invoke the existing
 * INT 0x80 syscall interface so our first user-mode code can do basic I/O.
 * ------------------------------------------------------------------------- */
/* INT 0x80 ABI:  EAX=syscall#  EBX=arg1  ECX=arg2  EDX=arg3  ESI=arg4
 * These tiny wrappers are compiled into the kernel binary but called from
 * ring-3 code (user_init).  Because all pages 0-4 MB have U/S=1 (set in
 * setup_paging), ring-3 can execute kernel-binary code and the CPU will
 * cross-call into the ring-0 syscall handler via INT 0x80. */
static int u_sys_write(unsigned int fd, const char* buf, unsigned int count) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"((unsigned int)buf), "d"(count)
        : "memory"
    );
    return ret;
}

static int u_sys_read(unsigned int fd, char* buf, unsigned int count) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ), "b"(fd), "c"((unsigned int)buf), "d"(count)
        : "memory"
    );
    return ret;
}

static void u_sys_exit(unsigned int code) {
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(SYS_EXIT), "b"(code)
        : "memory"
    );
}

/* First real ring-3 program.
 *
 * Execution flow:
 *   kernel_c_init() calls enter_user_mode() which iretd's into here.
 *   From this point on, the CPU is in ring 3.  All access to kernel
 *   services goes through INT 0x80.
 *
 * We print a banner via sys_write, then run a tiny echo loop. */
static void user_init(void) {
    const char banner[] =
        "\n*** Ring-3 user_init running ***\n"
        "[u-shell] keyboard echo (press keys):\n";
    u_sys_write(1, banner, sizeof(banner) - 1);

    /* Non-blocking keyboard echo loop.
     * sys_read on fd=0 returns immediately with 0 bytes if the kbd
     * buffer is empty, so we busy-spin (acceptable for a demo). */
    char c;
    for (;;) {
        if (u_sys_read(0, &c, 1) > 0) {
            u_sys_write(1, &c, 1);
        }
    }

    /* Unreachable, but keeps the compiler happy. */
    u_sys_exit(0);
}

#include "arch.h"
#include "core/smp.h"

extern void enter_user_mode(unsigned int user_eip, unsigned int user_esp);
/* =============================================================================
 * kernel_c_init()
 * =============================================================================
 * Called from kernel.asm BEFORE sti (interrupts still disabled).
 *
 * Initializes all C subsystems in dependency order:
 *   1. VMM       -- high-level virtual memory helpers
 *   2. Heap      -- dynamic allocation (malloc/free)
 *   3. Scheduler -- scheduling queues (MLFQ + CFS)
 *   4. Process   -- kernel process (PID 0)
 *   5. Syscalls  -- INT 0x80 handler table
 *   6. Block I/O -- ATA PIO driver
 *   7. Enable scheduler ticking in PIT handler
 *
 * After this returns, kernel.asm enables interrupts (sti) and enters
 * the console loop. The PIT will now safely call scheduler_tick().
 * =========================================================================== */
/* Minimal VGA helper for boot-time progress messages.
 * row: 0-based row index, text: null-terminated string, attr: VGA attribute. */
static void boot_log_line(unsigned int row, const char* text, unsigned char attr) {
    volatile unsigned short* vga = (unsigned short*)0xB8000;
    unsigned int col = 0;
    while (text[col] != 0 && col < 80) {
        vga[row * 80 + col] = (unsigned short)(((unsigned int)attr << 8) |
                                               (unsigned char)text[col]);
        col++;
    }
}

void kernel_c_init(void) {
    node_init();
    gdt_init();

    /* Row 2: high-level init banner (green on black). */
    boot_log_line(2, "Init VFS...", 0x0A);

    /* Defensive init: build the default RAMFS tree here as well.
     * Some boot paths evolved quickly; doing it again guarantees the
     * filesystem tree exists before we set CWD or run shell commands. */
    ramfs_init();
    vfs_mount_root(ramfs_get_vfs_root());

    /* VMM currently builds on the low-level paging setup done in setup_paging().
     * Having an explicit call here keeps the layering clean and ready for
     * per-arch VMM backends later. */
    boot_log_line(3, "Init VMM...", 0x0A);
    vmm_init();

    boot_log_line(4, "Init heap...", 0x0A);
    heap_init();

    boot_log_line(5, "Init scheduler...", 0x0A);
    scheduler_init();

    boot_log_line(6, "Init process table...", 0x0A);
    process_init();

    scheduler_set_current(process_get_current());

    boot_log_line(7, "Init syscalls...", 0x0A);
    syscall_init();

    /* Block device and SimpleFS initialisation.
     * sector_buf and identify_buffer are now static so they live in BSS,
     * eliminating the previous boot-stack overflow that caused a CPU fault. */
    boot_log_line(8, "Init block/FS...", 0x0A);
    block_init();
    diskfs_init();
    devfs_init();
    /* virtio_net_init();  // disabled for now to avoid hangs on non-virtio setups */

    {
        /* Ensure expected login directories exist even if earlier init changed. */
        (void)ramfs_mkdir_at_path("/home");
        (void)ramfs_mkdir_at_path("/home/user");
        (void)ramfs_mkdir_at_path("/home/normal");

        /* Default CWD: prefer /home/user, then /home/normal, else /. */
        vfs_node_t* home = vfs_lookup("/home/user");
        if (!home || home->type != VFS_NODE_DIR)
            home = vfs_lookup("/home/normal");
        if (home && home->type == VFS_NODE_DIR) {
            vfs_set_cwd(home);
        } else {
            vfs_set_cwd(vfs_get_root());
        }
    }

    /* Step 7: NOW it's safe to let the PIT call scheduler_tick().
     * This sets the sched_enabled flag in pit.asm.
     * Before this point, PIT ticks only increment the counter. */
    arch_timer_init(100);  /* 100 Hz PIT scheduler tick */

    /* SMP DISABLED: smp_init() sends INIT-SIPI-SIPI to AP CPUs using the
     * trampoline placed at physical page 8 (0x8000).  Our kernel is loaded
     * starting at 0x3000, so page 8 (0x8000) sits inside the kernel binary.
     * QEMU honours the SIPI and the AP starts executing kernel code bytes as
     * raw 16-bit real-mode instructions, corrupting the kernel stack and
     * triggering the #DE kernel panic you were seeing.
     *
     * For single-CPU QEMU (-smp 1) SMP is not needed.  Re-enable only after
     * the trampoline is placed at a safe page below 0x3000 (e.g. page 2 = 0x2000)
     * and the LAPIC MMIO region (0xFEE00000) is mapped in the page tables. */
    /* smp_init(); */

    /* Stay in ring 0 and return to the kernel shell.
     * Ring-3 can be tested on demand via the "ring3" shell command
     * which calls cmd_ring3_test() below. */
    return;
}

#define USER_STACK_TOP 0x00800000

void tss_set_kernel_stack(unsigned int esp0);

void cmd_ring3_test(void) {
    tss_set_kernel_stack(0x90000);
    enter_user_mode((unsigned int)user_init, USER_STACK_TOP);
}
