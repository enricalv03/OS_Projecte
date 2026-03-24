/* ============================================================================
 * arch/mips/boot/boot.S — MIPS32 entry point stub
 * ============================================================================
 * Targets: QEMU -M malta, -cpu 24Kf
 * Placeholder only — not yet implemented.
 * ============================================================================ */

    .section .text._start
    .global  _start

_start:
    /* Set up stack pointer */
    lui   $sp, 0x8030    /* stack top at 0x80300000 */

    /* Zero BSS */
    la    $t0, __bss_start
    la    $t1, __bss_end
1:
    bge   $t0, $t1, 2f
    nop
    sw    $zero, 0($t0)
    addiu $t0, $t0, 4
    j     1b
    nop
2:
    /* Jump to C entry */
    jal   mips_kernel_main
    nop

    /* Spin */
3:
    wait
    j     3b
    nop
