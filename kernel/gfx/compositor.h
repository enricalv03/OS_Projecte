#ifndef COMPOSITOR_H
#define COMPOSITOR_H

/* =============================================================================
 * kernel/gfx/compositor.h  —  Minimal tiling compositor / window model
 * =============================================================================
 * Implements a two-layer graphical stack inspired by Hyprland:
 *
 *   surface_t      A window's backing buffer + position + z-order.
 *   compositor     A kernel task that tiles surfaces and forwards input.
 *
 * Tiling layout (master/stack):
 *   - One "master" surface occupies the left half of the screen.
 *   - Up to (MAX_SURFACES - 1) "stack" surfaces share the right half.
 *   - Floating surfaces are drawn on top of the tiled layout.
 *
 * This initial implementation draws directly to the framebuffer (single
 * front-buffer).  A back-buffer + fb_swap_buffers() swap is the next step.
 * =========================================================================== */

/* Maximum number of surfaces the compositor tracks. */
#define MAX_SURFACES  8

/* Surface flags */
#define SURF_FLAG_VISIBLE   (1u << 0)  /* draw this surface             */
#define SURF_FLAG_FOCUSED   (1u << 1)  /* receives keyboard input       */
#define SURF_FLAG_FLOATING  (1u << 2)  /* not subject to tiling layout  */

/* A compositor surface.
 * The backing pixel buffer is allocated by the creator (or managed by the
 * compositor) and freed on surface_destroy. */
typedef struct {
    /* Geometry (screen coordinates, pixels) */
    int  x, y;
    int  w, h;

    unsigned int  z_order;   /* higher = drawn later (on top)     */
    unsigned int  flags;
    unsigned int  owner_pid; /* PID of the process that owns this */

    /* Title (e.g. displayed in the window bar). */
    char title[32];

    /* Pixel buffer: w * h pixels, 32bpp ARGB.
     * Managed externally; compositor just reads from it. */
    const unsigned int* pixels;
} surface_t;

/* ---- Compositor API ------------------------------------------------------ */

/* Initialise the compositor (sets up fb, clears to background colour). */
void compositor_init(void);

/* Create a new surface and return its handle (index), or -1 on error. */
int compositor_surface_create(int x, int y, int w, int h,
                               unsigned int flags, unsigned int owner_pid,
                               const char* title);

/* Destroy a surface (marks it unused). */
void compositor_surface_destroy(int handle);

/* Update a surface's pixel buffer pointer (client has redrawn). */
void compositor_surface_set_pixels(int handle, const unsigned int* pixels);

/* Re-apply the tiling layout and redraw all visible surfaces. */
void compositor_redraw(void);

/* Handle a key event: route to the focused surface's owner process. */
void compositor_handle_key(unsigned char scancode);

/* Set focus to a specific surface handle. */
void compositor_focus(int handle);

/* Return the handle of the currently focused surface (-1 if none). */
int compositor_get_focused(void);

/* ---- Convenience: fill a surface with a solid colour --------------------- */
void compositor_fill_surface(int handle, unsigned int colour);

#endif
