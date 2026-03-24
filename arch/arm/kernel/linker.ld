/* ============================================================================
 * arch/arm/kernel/linker.ld  —  ARM linker script for QEMU -M virt
 * ============================================================================
 * QEMU -M virt places RAM at 0x40000000.  We use 0x40008000 as the load
 * address (the same address the QEMU Linux loader uses), which means QEMU
 * can boot the ELF directly with -kernel flag.
 * =========================================================================== */

ENTRY(_start)
OUTPUT_FORMAT("elf32-littlearm")

SECTIONS
{
    . = 0x40008000;

    /* _start must come first so the CPU begins there */
    .text : {
        *(.text._start)
        *(.text.vectors)
        *(.text .text.*)
    }

    .rodata : ALIGN(4) { *(.rodata .rodata.*) }

    .data : ALIGN(4) { *(.data .data.*) }

    /* BSS: zero-initialised at boot by boot.S */
    . = ALIGN(4);
    __bss_start = .;
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
    . = ALIGN(4);
    __bss_end = .;
}
