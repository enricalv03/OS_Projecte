#include "sched/process.h"

/*
 * ARM scheduler compatibility stubs.
 *
 * These weak symbols allow linking kernel/sched/scheduler.c into the ARM
 * skeleton build before the full process/context-switch subsystem is ported.
 * Once real ARM process management is linked, strong definitions override
 * these stubs automatically.
 */

__attribute__((weak))
pcb_t* process_get_by_pid(unsigned int pid) {
    (void)pid;
    return 0;
}

__attribute__((weak))
void process_terminate(pcb_t* proc) {
    (void)proc;
}

__attribute__((weak))
void context_switch(pcb_t* from, pcb_t* to) {
    extern void context_switch_asm(pcb_t* from, pcb_t* to);
    if (!to) {
        return;
    }
    context_switch_asm(from, to);
}
