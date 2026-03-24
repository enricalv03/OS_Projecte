/* RISC-V linker script (QEMU -M virt, RAM at 0x80000000) */
ENTRY(_start)
OUTPUT_FORMAT("elf32-littleriscv")

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
