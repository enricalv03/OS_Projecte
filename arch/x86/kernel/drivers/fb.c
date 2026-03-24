/* =============================================================================
 * arch/x86/kernel/drivers/fb.c  —  Bochs VGA/BGA linear framebuffer driver
 * =============================================================================
 * The Bochs Graphics Adapter (BGA) is the default VGA device in QEMU.
 * It exposes a simple I/O-port register interface at 0x01CE / 0x01CF that
 * allows setting any resolution and bpp without BIOS calls, and provides a
 * linear framebuffer at a PCI BAR address.
 *
 * We use the well-known QEMU default framebuffer address 0xFD000000.
 * If you want portability, scan the PCI bus for the VGA device's BAR0
 * instead (future enhancement).
 * =========================================================================== */

#include "fb.h"
#include "memory/vmm.h"

/* ---- BGA I/O port register interface ------------------------------------- */
#define BGA_PORT_INDEX   0x01CE
#define BGA_PORT_DATA    0x01CF

/* BGA register indices */
#define BGA_REG_ID          0x00
#define BGA_REG_XRES        0x01
#define BGA_REG_YRES        0x02
#define BGA_REG_BPP         0x03
#define BGA_REG_ENABLE      0x04
#define BGA_REG_BANK        0x05
#define BGA_REG_VIRT_WIDTH  0x06
#define BGA_REG_VIRT_HEIGHT 0x07
#define BGA_REG_X_OFFSET    0x08
#define BGA_REG_Y_OFFSET    0x09

/* BGA_REG_ENABLE flags */
#define BGA_ENABLE_DISABLED 0x00
#define BGA_ENABLE_ENABLED  0x01
#define BGA_ENABLE_LFB      0x40  /* use linear frame buffer */
#define BGA_ENABLE_NOCLEAR  0x80  /* do not clear on mode set */

/* Expected ID range (0xB0C0 – 0xB0C5 depending on BGA version) */
#define BGA_ID_MIN  0xB0C0
#define BGA_ID_MAX  0xB0C5

/* ---- Physical address of the linear framebuffer in QEMU with VGA std ----- */
/* This is the PCI BAR0 of the QEMU VGA device.  Hardcoded for simplicity;
 * a PCI scan would find it automatically. */
#define FB_PHYS_ADDR   0xFD000000u
#define FB_MAP_VIRT    0xFC000000u   /* virtual address we map it to */

#define FB_MAX_BYTES   (1920u * 1080u * 4u)   /* 8 MB for 1920×1080 32bpp */

/* ---- I/O helpers (x86 inb/outb) ----------------------------------------- */

static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile("outw %0, %1" :: "a"(val), "Nd"(port));
}
static inline unsigned short inw(unsigned short port) {
    unsigned short val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* ---- BGA register access ------------------------------------------------- */

static void bga_write(unsigned short reg, unsigned short val) {
    outw(BGA_PORT_INDEX, reg);
    outw(BGA_PORT_DATA,  val);
}

static unsigned short bga_read(unsigned short reg) {
    outw(BGA_PORT_INDEX, reg);
    return inw(BGA_PORT_DATA);
}

/* ---- Driver state -------------------------------------------------------- */

static unsigned int  fb_width;
static unsigned int  fb_height;
static unsigned int  fb_bpp;
static unsigned int* fb_virt;    /* virtual address of linear framebuffer */
static unsigned int  fb_ready;

/* ---- fb_init ------------------------------------------------------------- */

int fb_init(unsigned int width, unsigned int height, unsigned int bpp) {
    /* Check BGA is present */
    unsigned short id = bga_read(BGA_REG_ID);
    if (id < BGA_ID_MIN || id > BGA_ID_MAX) {
        return 0;   /* no BGA detected */
    }

    if (bpp != 32) return 0;   /* only 32bpp supported for now */

    /* Disable BGA while reconfiguring */
    bga_write(BGA_REG_ENABLE, BGA_ENABLE_DISABLED);

    bga_write(BGA_REG_XRES,        (unsigned short)width);
    bga_write(BGA_REG_YRES,        (unsigned short)height);
    bga_write(BGA_REG_BPP,         (unsigned short)bpp);
    bga_write(BGA_REG_VIRT_WIDTH,  (unsigned short)width);
    bga_write(BGA_REG_VIRT_HEIGHT, (unsigned short)(height * 2)); /* double-buffer */
    bga_write(BGA_REG_X_OFFSET, 0);
    bga_write(BGA_REG_Y_OFFSET, 0);

    /* Enable: linear framebuffer + clear on mode set */
    bga_write(BGA_REG_ENABLE, BGA_ENABLE_ENABLED | BGA_ENABLE_LFB);

    /* Map the physical framebuffer into our virtual address space.
     * We identity-map a contiguous range of 4 KB pages starting at
     * FB_MAP_VIRT → FB_PHYS_ADDR. */
    unsigned int bytes = width * height * 4u;
    unsigned int pages = (bytes + 4095u) >> 12;
    if (pages > (FB_MAX_BYTES >> 12)) pages = FB_MAX_BYTES >> 12;

    for (unsigned int i = 0; i < pages; i++) {
        unsigned int virt = FB_MAP_VIRT  + (i << 12);
        unsigned int phys = FB_PHYS_ADDR + (i << 12);
        vmm_map_page(virt, phys, VMM_KERNEL_RW);
    }

    fb_virt   = (unsigned int*)FB_MAP_VIRT;
    fb_width  = width;
    fb_height = height;
    fb_bpp    = bpp;
    fb_ready  = 1;

    /* Clear to black */
    fb_clear(FB_BLACK);
    return 1;
}

unsigned int fb_get_width(void)  { return fb_width;  }
unsigned int fb_get_height(void) { return fb_height; }
unsigned int fb_get_bpp(void)    { return fb_bpp;    }

/* ---- Drawing primitives -------------------------------------------------- */

void fb_put_pixel(int x, int y, fb_color_t colour) {
    if (!fb_ready) return;
    if ((unsigned int)x >= fb_width || (unsigned int)y >= fb_height) return;
    fb_virt[(unsigned int)y * fb_width + (unsigned int)x] = colour;
}

fb_color_t fb_get_pixel(int x, int y) {
    if (!fb_ready) return 0;
    if ((unsigned int)x >= fb_width || (unsigned int)y >= fb_height) return 0;
    return fb_virt[(unsigned int)y * fb_width + (unsigned int)x];
}

void fb_clear(fb_color_t colour) {
    if (!fb_ready) return;
    unsigned int total = fb_width * fb_height;
    for (unsigned int i = 0; i < total; i++)
        fb_virt[i] = colour;
}

void fb_draw_rect(int x, int y, int w, int h, fb_color_t colour) {
    if (!fb_ready) return;
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            fb_put_pixel(col, row, colour);
        }
    }
}

void fb_draw_rect_outline(int x, int y, int w, int h, fb_color_t colour) {
    if (!fb_ready) return;
    /* Top and bottom */
    for (int col = x; col < x + w; col++) {
        fb_put_pixel(col, y,         colour);
        fb_put_pixel(col, y + h - 1, colour);
    }
    /* Left and right */
    for (int row = y + 1; row < y + h - 1; row++) {
        fb_put_pixel(x,         row, colour);
        fb_put_pixel(x + w - 1, row, colour);
    }
}

void fb_blit_bitmap(int x, int y, int w, int h, const unsigned int* src) {
    if (!fb_ready || !src) return;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            fb_put_pixel(x + col, y + row, src[row * w + col]);
        }
    }
}

void fb_swap_buffers(void) {
    /* With a single front-buffer this is a no-op; the pixels are written
     * directly to the mapped linear framebuffer.  A back-buffer
     * implementation would memcpy a shadow buffer here. */
    (void)fb_ready;
}
