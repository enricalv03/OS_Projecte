#ifndef FB_H
#define FB_H

/* =============================================================================
 * arch/x86/kernel/drivers/fb.h  —  Linear framebuffer driver (Bochs VGA / BGA)
 * =============================================================================
 * Provides a pixel-level drawing API backed by the Bochs Graphics Adapter
 * (BGA), which QEMU exposes when run with the default VGA option (-vga std).
 *
 * The BGA I/O register interface works in protected mode without any BIOS
 * calls, making it ideal for a bare-metal kernel.
 *
 * Supported modes:  any resolution up to 1920×1080 at 32 bpp.
 * Default mode:     800×600×32bpp (safe for QEMU on any host resolution).
 * =========================================================================== */

/* Pixel colour — packed 32-bit ARGB (alpha byte ignored by hardware). */
typedef unsigned int fb_color_t;

/* Convenience colour constructors */
#define FB_RGB(r, g, b)  (((unsigned int)(r) << 16) | \
                          ((unsigned int)(g) <<  8) | \
                          ((unsigned int)(b)))

/* A few named colours */
#define FB_BLACK    FB_RGB(  0,   0,   0)
#define FB_WHITE    FB_RGB(255, 255, 255)
#define FB_RED      FB_RGB(255,  64,  64)
#define FB_GREEN    FB_RGB( 80, 220, 100)
#define FB_BLUE     FB_RGB( 60, 120, 240)
#define FB_DARK_BG  FB_RGB( 24,  24,  32)
#define FB_ACCENT   FB_RGB( 80, 160, 255)

/* ---- Framebuffer init & info --------------------------------------------- */

/* Initialise the BGA, map the framebuffer, and switch to graphics mode.
 * width×height must be <= 1920×1080; bpp must be 32.
 * Returns 1 on success, 0 if the BGA is not detected (e.g. no QEMU VGA). */
int fb_init(unsigned int width, unsigned int height, unsigned int bpp);

/* Return the current framebuffer dimensions. */
unsigned int fb_get_width(void);
unsigned int fb_get_height(void);
unsigned int fb_get_bpp(void);

/* ---- Pixel-level drawing ------------------------------------------------- */

/* Write one pixel.  (x, y) are in screen coordinates; no clipping. */
void fb_put_pixel(int x, int y, fb_color_t colour);

/* Read one pixel. */
fb_color_t fb_get_pixel(int x, int y);

/* ---- Primitives ---------------------------------------------------------- */

/* Fill the entire framebuffer with a single colour. */
void fb_clear(fb_color_t colour);

/* Draw a filled axis-aligned rectangle. */
void fb_draw_rect(int x, int y, int w, int h, fb_color_t colour);

/* Draw a single-pixel-wide rectangle border. */
void fb_draw_rect_outline(int x, int y, int w, int h, fb_color_t colour);

/* Blit a 32-bpp bitmap from src (row-major, width × height pixels) at (x, y).
 * src must point to width*height unsigned int values. */
void fb_blit_bitmap(int x, int y, int w, int h, const unsigned int* src);

/* ---- Buffering ----------------------------------------------------------- */

/* Present the back-buffer to the screen by copying it to the linear
 * framebuffer (or by swapping the virtual Y-offset if double-buffering is
 * available via the BGA's virtual-height register).
 * For the initial implementation this is a simple memcpy. */
void fb_swap_buffers(void);

#endif
