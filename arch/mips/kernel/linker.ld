/* MIPS32 linker script (QEMU -M malta, RAM at 0x80000000 KSEG0) */
ENTRY(_start)

SECTIONS {
    . = 0x80000000;
    .text   : { *(.text._start) *(.text .text.*) }
    .rodata : ALIGN(4) { *(.rodata .rodata.*) }
    .data   : ALIGN(4) { *(.data .data.*) }
    . = ALIGN(4);
    __bss_start = .;
    .bss    : { *(.bss .bss.*) *(COMMON) }
    . = ALIGN(4);
    __bss_end = .;
}
