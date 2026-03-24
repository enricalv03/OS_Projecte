/* MIPS kernel entry stub */
#include "arch.h"

void mips_kernel_main(void) {
    arch_disable_interrupts();
    for (;;) arch_halt();
}
