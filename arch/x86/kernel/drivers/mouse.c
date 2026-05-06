#include "drivers/mouse.h"
#include "drivers/fb.h"

/* PS/2 controller ports */
#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_CMD     0x64

/* PIC ports */
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

static inline unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void ps2_wait_input(void) {
    int timeout = 100000;
    while (timeout--) {
        if (!(inb(PS2_STATUS) & 0x02))
            return;
    }
}

static void ps2_wait_output(void) {
    int timeout = 100000;
    while (timeout--) {
        if (inb(PS2_STATUS) & 0x01)
            return;
    }
}

static void ps2_write_mouse(unsigned char byte) {
    ps2_wait_input();
    outb(PS2_CMD, 0xD4);
    ps2_wait_input();
    outb(PS2_DATA, byte);
}

static unsigned char ps2_read(void) {
    ps2_wait_output();
    return inb(PS2_DATA);
}

static volatile int mouse_x;
static volatile int mouse_y;
static volatile int mouse_buttons;

static int screen_w;
static int screen_h;

static unsigned char mouse_cycle;
static signed char   mouse_byte[3];

void mouse_init(void) {
    screen_w = (int)fb_get_width();
    screen_h = (int)fb_get_height();
    if (screen_w == 0) screen_w = 800;
    if (screen_h == 0) screen_h = 600;

    mouse_x = screen_w / 2;
    mouse_y = screen_h / 2;
    mouse_buttons = 0;
    mouse_cycle = 0;

    /* Enable the auxiliary (mouse) PS/2 port */
    ps2_wait_input();
    outb(PS2_CMD, 0xA8);

    /* Get Compaq status byte, enable IRQ12 */
    ps2_wait_input();
    outb(PS2_CMD, 0x20);
    unsigned char status = ps2_read();
    status |= 0x02;   /* enable IRQ12 */
    status &= ~0x20;  /* enable mouse clock */
    ps2_wait_input();
    outb(PS2_CMD, 0x60);
    ps2_wait_input();
    outb(PS2_DATA, status);

    /* Use default settings */
    ps2_write_mouse(0xF6);
    ps2_read(); /* ACK */

    /* Enable data reporting */
    ps2_write_mouse(0xF4);
    ps2_read(); /* ACK */

    /* Unmask IRQ12 on the slave PIC and IRQ2 (cascade) on the master */
    outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << 4));
    outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << 2));
}

void mouse_irq_handler(void);

void mouse_irq_handler(void) {
    unsigned char data = inb(PS2_DATA);

    switch (mouse_cycle) {
    case 0:
        mouse_byte[0] = (signed char)data;
        if (data & 0x08) /* bit 3 must always be set in byte 0 */
            mouse_cycle = 1;
        break;
    case 1:
        mouse_byte[1] = (signed char)data;
        mouse_cycle = 2;
        break;
    case 2:
        mouse_byte[2] = (signed char)data;
        mouse_cycle = 0;

        mouse_buttons = mouse_byte[0] & 0x07;

        int dx = mouse_byte[1];
        int dy = mouse_byte[2];

        mouse_x += dx;
        mouse_y -= dy; /* PS/2 Y is inverted vs screen coords */

        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x >= screen_w) mouse_x = screen_w - 1;
        if (mouse_y >= screen_h) mouse_y = screen_h - 1;
        break;
    }

    /* EOI to slave then master PIC */
    outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}

void mouse_get_state(mouse_state_t* out) {
    out->x       = mouse_x;
    out->y       = mouse_y;
    out->buttons = mouse_buttons;
}

int mouse_get_x(void)       { return mouse_x; }
int mouse_get_y(void)       { return mouse_y; }
int mouse_get_buttons(void) { return mouse_buttons; }
