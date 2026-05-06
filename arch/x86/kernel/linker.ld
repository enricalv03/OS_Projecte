ENTRY(start)

SECTIONS
{
  . = 0x3000;

  .text : {
    *(.text)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data)
  }

  /* C++ global constructors (called by _init_cxx in kernel.asm) */
  .init_array : {
    __init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;
  }

  /* C++ global destructors (not normally called in a kernel) */
  .fini_array : {
    __fini_array_start = .;
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    __fini_array_end = .;
  }

  /* BSS: zero-initialized data. The kernel startup code uses
   * __bss_start and __bss_end to clear this region before any
   * C code runs, since nobody else zeroes it for us. */
  .bss : {
    __bss_start = .;
    *(.bss)
    *(COMMON)
    __bss_end = .;
  }

  /* Total kernel size (useful for loader calculations) */
  __kernel_end = .;
}