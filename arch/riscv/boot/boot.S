/* ============================================================================
 * arch/riscv/boot/boot.S — RISC-V (RV32IMC) entry point stub
 * ============================================================================
 * Targets: QEMU -M virt, -cpu rv32 (or rv64 with appropriate flags).
 *
 * Placeholder only — not yet implemented.
 * To build:  ARCH=riscv make  (requires riscv32-unknown-elf-gcc toolchain)
 * ============================================================================ */

    .section .text._start
    .global  _start

_start:
    /* Set up stack (QEMU virt RISC-V RAM starts at 0x80000000) */
    li    sp, 0x80100000        /* 1 MB stack */

    /* Zero BSS */
    la    a0, __bss_start
    la    a1, __bss_end
1:
    bge   a0, a1, 2f
    sw    zero, 0(a0)
    addi  a0, a0, 4
    j     1b
2:
    /* Call C entry point */
    call  riscv_kernel_main

    /* Spin */
3:
    wfi
    j     3b
