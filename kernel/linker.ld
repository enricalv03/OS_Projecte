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