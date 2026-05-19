#include "sched/process.h"

/*
 * ARM context switch glue.
 *
 * ARM bring-up links the generic scheduler but uses an ARM-specific ASM
 * context switch routine.
 */

void context_switch(pcb_t* from, pcb_t* to) {
    extern void context_switch_asm(pcb_t* from, pcb_t* to);
    if (!to) {
        return;
    }
    context_switch_asm(from, to);
}
