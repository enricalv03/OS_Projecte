#include "arch.h"
#include "panic.h"

/* Print helpers from the ASM screen driver */
extern void print_string_pm(const char* s, void* edi, unsigned char colour);
extern void print_hex32_pm(unsigned int val, void* edi, unsigned char colour);
extern void print_char_pm(unsigned char c, void* edi, unsigned char colour);

/* CR2 saved by exceptions.asm before calling fault_handler */
extern unsigned int saved_cr2;

/* VGA text buffer base */
#define VGA_BASE 0xB8000

static unsigned int panic_col;   /* current VGA byte offset (x2 per char) */

static void vga_puts(const char* s, unsigned char attr) {
    unsigned short* vga = (unsigned short*)VGA_BASE;
    while (*s) {
        vga[panic_col / 2] = (unsigned short)(((unsigned int)attr << 8) | (unsigned char)*s);
        panic_col += 2;
        s++;
    }
}

static void vga_puthex(unsigned int v, unsigned char attr) {
    static const char hex[] = "0123456789ABCDEF";
    char buf[11];
    buf[0]  = '0'; buf[1] = 'x';
    buf[2]  = hex[(v >> 28) & 0xF];
    buf[3]  = hex[(v >> 24) & 0xF];
    buf[4]  = hex[(v >> 20) & 0xF];
    buf[5]  = hex[(v >> 16) & 0xF];
    buf[6]  = hex[(v >> 12) & 0xF];
    buf[7]  = hex[(v >>  8) & 0xF];
    buf[8]  = hex[(v >>  4) & 0xF];
    buf[9]  = hex[(v >>  0) & 0xF];
    buf[10] = 0;
    vga_puts(buf, attr);
}

static void vga_newline(void) {
    /* Advance to the next 80-column row (160 bytes) */
    unsigned int col_chars = (panic_col / 2) % 80;
    panic_col += (80 - col_chars) * 2;
}

/* Exception names for vectors 0-31 */
static const char* const exc_names[32] = {
    "#DE Divide Error",       "#DB Debug",
    "NMI",                    "#BP Breakpoint",
    "#OF Overflow",           "#BR Bound Range",
    "#UD Invalid Opcode",     "#NM No FPU",
    "#DF Double Fault",       "Coprocessor Overrun",
    "#TS Invalid TSS",        "#NP Seg Not Present",
    "#SS Stack Fault",        "#GP General Protection",
    "#PF Page Fault",         "Reserved",
    "#MF x87 FPU",            "#AC Alignment Check",
    "#MC Machine Check",      "#XM SIMD FP",
    "#VE Virtualization",     "Reserved",
    "Reserved",               "Reserved",
    "Reserved",               "Reserved",
    "Reserved",               "Reserved",
    "Reserved",               "Reserved",
    "#SX Security",           "Reserved"
};

void fault_handler(isr_frame_t* f) {
    arch_disable_interrupts();

    /* Paint the top 7 rows of the VGA screen for the panic display */
    unsigned short* vga = (unsigned short*)VGA_BASE;
    for (unsigned int i = 0; i < 80 * 7; i++) {
        vga[i] = (unsigned short)(0x4F00 | ' ');   /* red background */
    }
    panic_col = 0;

    /* Row 0: banner */
    vga_puts("*** KERNEL PANIC ***  ", 0x4F);
    {
        const char* name = (f->vec < 32) ? exc_names[f->vec] : "Unknown";
        vga_puts(name, 0x4E);
    }
    vga_puts("  vec=", 0x4F);
    vga_puthex(f->vec, 0x4F);
    vga_puts("  err=", 0x4F);
    vga_puthex(f->err, 0x4F);

    /* Row 1: EIP / CS / EFLAGS */
    vga_newline();
    vga_puts("EIP=", 0x4F); vga_puthex(f->eip,    0x4F);
    vga_puts(" CS=", 0x4F); vga_puthex(f->cs,     0x4F);
    vga_puts(" EFLAGS=", 0x4F); vga_puthex(f->eflags, 0x4F);

    /* Row 2: EAX/EBX/ECX/EDX */
    vga_newline();
    vga_puts("EAX=", 0x4F); vga_puthex(f->eax, 0x4F);
    vga_puts(" EBX=", 0x4F); vga_puthex(f->ebx, 0x4F);
    vga_puts(" ECX=", 0x4F); vga_puthex(f->ecx, 0x4F);
    vga_puts(" EDX=", 0x4F); vga_puthex(f->edx, 0x4F);

    /* Row 3: ESI/EDI/EBP/ESP */
    vga_newline();
    vga_puts("ESI=", 0x4F); vga_puthex(f->esi, 0x4F);
    vga_puts(" EDI=", 0x4F); vga_puthex(f->edi, 0x4F);
    vga_puts(" EBP=", 0x4F); vga_puthex(f->ebp, 0x4F);
    /* If ring-3 fault (CS & 3 != 0), user_esp is valid */
    if (f->cs & 3) {
        vga_puts(" UESP=", 0x4F); vga_puthex(f->user_esp, 0x4F);
        vga_puts(" USS=",  0x4F); vga_puthex(f->user_ss,  0x4F);
    }

    /* Row 4: CR2 (always show — irrelevant for non-PF but harmless) */
    vga_newline();
    vga_puts("CR2=", 0x4F); vga_puthex(saved_cr2, 0x4F);
    if (f->vec == 14) {
        vga_puts("  (Page Fault address)", 0x4E);
    }

    /* Row 5: kernel halted */
    vga_newline();
    vga_puts("System halted. Reset required.", 0x4C);

    arch_halt();
}

void kernel_panic(void) {
    arch_disable_interrupts();
    arch_halt();
}
