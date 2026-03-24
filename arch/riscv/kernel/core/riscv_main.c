/* ============================================================================
 * arch/riscv/kernel/core/riscv_main.c — RISC-V C entry point stub
 * ============================================================================
 * Placeholder. When the toolchain and QEMU target are available:
 *   make ARCH=riscv run-riscv
 * ============================================================================ */

#include "arch.h"

void riscv_kernel_main(void) {
    /* TODO: initialise PMM, VMM, then call kernel_c_init() */
    arch_disable_interrupts();
    for (;;) arch_halt();
}
