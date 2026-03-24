#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

/* Stop the kernel: disable interrupts and halt the CPU. Never returns. */
void kernel_panic(void);

/* -------------------------------------------------------------------------
 * isr_frame_t — layout of the stack inside isr_common (exceptions.asm).
 * The C fault_handler receives a pointer to this frame.
 * -------------------------------------------------------------------------
 * Offsets match the pusha + ISR-stub + CPU-push layout exactly:
 *   pusha  saves: edi esi ebp esp(dummy) ebx edx ecx eax  (32 bytes)
 *   ISR stub:     vec err                                  (+8  bytes)
 *   CPU push:     eip cs eflags                            (+12 bytes)
 *   Ring-3 only:  user_esp user_ss                         (+8  bytes)
 * ------------------------------------------------------------------------- */
typedef struct {
    /* pusha block — lowest address first */
    unsigned int edi, esi, ebp, esp_dummy;
    unsigned int ebx, edx, ecx, eax;
    /* ISR stub */
    unsigned int vec;
    unsigned int err;
    /* CPU-pushed exception frame */
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
    /* present only when privilege level changed (ring-3 fault) */
    unsigned int user_esp;
    unsigned int user_ss;
} __attribute__((packed)) isr_frame_t;

/* Called from exceptions.asm; prints register dump then halts. */
void fault_handler(isr_frame_t* f);

#endif
