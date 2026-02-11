#ifndef KERNEL_H
#define KERNEL_H

/* =============================================================================
 * Kernel Core Header
 * =============================================================================
 * Central include for kernel-wide declarations and configuration constants.
 * =========================================================================== */

/* Kernel version */
#define KERNEL_VERSION_MAJOR  0
#define KERNEL_VERSION_MINOR  2
#define KERNEL_VERSION_PATCH  0

/* Memory layout constants (must match linker.ld and stage2.asm) */
#define KERNEL_LOAD_ADDR      0x3000
#define KERNEL_STACK_TOP      0x90000
#define VGA_TEXT_BASE          0xB8000

/* Console configuration */
#define CONSOLE_START_ROW     14
#define CONSOLE_COLS          80
#define CONSOLE_ROWS          25

/* ---- Subsystem initialization (called from kernel.asm) ---- */

/* Master initialization function: sets up heap, processes, scheduler,
 * syscalls, and block devices. Called after paging, IDT, PIC, PIT,
 * PMM, and RAMFS are already initialized in ASM. */
void kernel_c_init(void);

#endif
