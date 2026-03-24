/* =============================================================================
 * kernel/gfx/compositor.c  —  Minimal tiling compositor
 * =============================================================================
 * Layout philosophy (Hyprland-inspired):
 *   - Background fill in a dark colour.
 *   - Tiled surfaces arranged in master/stack columns.
 *   - Floating surfaces drawn last (always on top).
 *   - A 1-px accent-coloured border on the focused surface.
 *   - 4 px gaps between windows and from screen edges.
 * =========================================================================== */

#include "compositor.h"
#include "drivers/fb.h"
#include "lib/kstring.h"

/* ---- Internal state ------------------------------------------------------ */

#define GAP  4   /* pixel gap between tiles and screen edge */

static surface_t surfaces[MAX_SURFACES];
static unsigned int surf_count = 0;
static int focused_handle = -1;

/* Accent palette */
#define COL_BG       FB_DARK_BG
#define COL_BORDER   FB_ACCENT
#define COL_WIN_BG   FB_RGB(32, 32, 44)
#define COL_BAR      FB_RGB(20, 20, 30)
#define COL_TEXT     FB_WHITE

/* Height of the window title bar in pixels */
#define BAR_H  16

/* ---- Internal helpers ---------------------------------------------------- */

/* Draw a simple 8×8 bitmap glyph for ASCII characters 0x20-0x7E.
 * This is a minimal 1-bit font encoded as 8 bytes per glyph. */

/* 8×8 font data for printable ASCII (generated from a standard VGA font ROM).
 * Only a minimal subset is inlined here — extend as needed. */
static const unsigned char font8x8[][8] = {
    /* space */ { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
    /* !     */ { 0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00 },
    /* "     */ { 0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00 },
    /* #     */ { 0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00 },
    /* $ ... */ { 0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00 },
    /* %     */ { 0x00,0x66,0xAC,0xD8,0x36,0x6A,0xCC,0x00 },
    /* &     */ { 0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00 },
    /* '     */ { 0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00 },
    /* (     */ { 0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00 },
    /* )     */ { 0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00 },
    /* *     */ { 0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00 },
    /* +     */ { 0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00 },
    /* ,     */ { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30 },
    /* -     */ { 0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00 },
    /* .     */ { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00 },
    /* /     */ { 0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00 },
    /* 0     */ { 0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00 },
    /* 1     */ { 0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00 },
    /* 2     */ { 0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00 },
    /* 3     */ { 0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00 },
    /* 4     */ { 0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00 },
    /* 5     */ { 0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00 },
    /* 6     */ { 0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00 },
    /* 7     */ { 0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00 },
    /* 8     */ { 0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00 },
    /* 9     */ { 0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00 },
    /* :     */ { 0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00 },
    /* ;     */ { 0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30 },
    /* <     */ { 0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00 },
    /* =     */ { 0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00 },
    /* >     */ { 0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00 },
    /* ?     */ { 0x3C,0x66,0x06,0x1C,0x18,0x00,0x18,0x00 },
    /* @     */ { 0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00 },
    /* A-Z, a-z, symbols as placeholders (extend with real font data): */
};

/* Total entries above */
#define FONT_GLYPHS  ((unsigned int)(sizeof(font8x8) / 8))

static void draw_char(int px, int py, char c, unsigned int fg) {
    unsigned int idx = (unsigned int)(unsigned char)c;
    if (idx < 0x20 || idx > 0x20 + FONT_GLYPHS - 1u) idx = 0x20; /* space */
    idx -= 0x20;
    if (idx >= FONT_GLYPHS) return;

    for (int row = 0; row < 8; row++) {
        unsigned char bits = font8x8[idx][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80u >> (unsigned)col))
                fb_put_pixel(px + col, py + row, fg);
        }
    }
}

static void draw_string(int px, int py, const char* s, unsigned int fg) {
    for (; *s; s++, px += 8)
        draw_char(px, py, *s, fg);
}

/* ---- compositor_init ----------------------------------------------------- */

void compositor_init(void) {
    unsigned int i;
    for (i = 0; i < MAX_SURFACES; i++) {
        surfaces[i].flags = 0;  /* not visible = free slot */
    }
    surf_count    = 0;
    focused_handle = -1;

    /* Switch to 800×600×32 graphics mode (BGA).  If it fails we remain in
     * text mode and the compositor becomes a no-op. */
    fb_init(800, 600, 32);

    /* Paint background */
    fb_clear(COL_BG);
}

/* ---- surface management -------------------------------------------------- */

int compositor_surface_create(int x, int y, int w, int h,
                               unsigned int flags, unsigned int owner_pid,
                               const char* title) {
    if (surf_count >= MAX_SURFACES) return -1;

    /* Find a free slot */
    int handle = -1;
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (!(surfaces[i].flags & SURF_FLAG_VISIBLE)) {
            handle = i;
            break;
        }
    }
    if (handle < 0) return -1;

    surface_t* s = &surfaces[handle];
    s->x         = x;
    s->y         = y;
    s->w         = w;
    s->h         = h;
    s->z_order   = surf_count;
    s->flags     = flags | SURF_FLAG_VISIBLE;
    s->owner_pid = owner_pid;
    s->pixels    = 0;

    /* Copy title (truncate at 31 chars) */
    unsigned int i;
    for (i = 0; i < 31 && title && title[i]; i++)
        s->title[i] = title[i];
    s->title[i] = '\0';

    surf_count++;

    /* Auto-focus first surface */
    if (focused_handle < 0)
        focused_handle = handle;

    return handle;
}

void compositor_surface_destroy(int handle) {
    if (handle < 0 || handle >= MAX_SURFACES) return;
    surfaces[handle].flags = 0;
    surf_count = surf_count > 0 ? surf_count - 1 : 0;
    if (focused_handle == handle) focused_handle = -1;
}

void compositor_surface_set_pixels(int handle, const unsigned int* pixels) {
    if (handle < 0 || handle >= MAX_SURFACES) return;
    surfaces[handle].pixels = pixels;
}

/* ---- Tiling layout ------------------------------------------------------- */

/* Apply a master/stack tiling layout to non-floating surfaces. */
static void apply_tiling_layout(void) {
    unsigned int sw = fb_get_width();
    unsigned int sh = fb_get_height();
    if (sw == 0 || sh == 0) return;

    /* Collect tiled surface handles */
    int tiled[MAX_SURFACES];
    int tiled_count = 0;
    for (int i = 0; i < MAX_SURFACES; i++) {
        if ((surfaces[i].flags & SURF_FLAG_VISIBLE) &&
            !(surfaces[i].flags & SURF_FLAG_FLOATING)) {
            tiled[tiled_count++] = i;
        }
    }
    if (tiled_count == 0) return;

    int gap = GAP;

    if (tiled_count == 1) {
        /* Single tile: full screen with gap */
        surface_t* s = &surfaces[tiled[0]];
        s->x = gap;
        s->y = gap;
        s->w = (int)sw - 2 * gap;
        s->h = (int)sh - 2 * gap;
        return;
    }

    /* Master occupies the left half */
    int master_w = (int)sw / 2 - gap - gap / 2;
    int master_h = (int)sh - 2 * gap;

    surface_t* master = &surfaces[tiled[0]];
    master->x = gap;
    master->y = gap;
    master->w = master_w;
    master->h = master_h;

    /* Stack: share the right half equally */
    int stack_count = tiled_count - 1;
    int stack_x     = gap + master_w + gap;
    int stack_w     = (int)sw - stack_x - gap;
    int stack_unit  = ((int)sh - 2 * gap - (stack_count - 1) * gap) / stack_count;

    for (int i = 0; i < stack_count; i++) {
        surface_t* s = &surfaces[tiled[i + 1]];
        s->x = stack_x;
        s->y = gap + i * (stack_unit + gap);
        s->w = stack_w;
        s->h = stack_unit;
    }
}

/* ---- Rendering ----------------------------------------------------------- */

static void render_surface(int handle) {
    surface_t* s = &surfaces[handle];
    if (!(s->flags & SURF_FLAG_VISIBLE)) return;

    /* Window background */
    fb_draw_rect(s->x, s->y, s->w, s->h, COL_WIN_BG);

    /* Title bar */
    fb_draw_rect(s->x, s->y, s->w, BAR_H, COL_BAR);
    draw_string(s->x + 4, s->y + (BAR_H - 8) / 2, s->title, COL_TEXT);

    /* Client area: blit pixel buffer if available */
    if (s->pixels) {
        fb_blit_bitmap(s->x, s->y + BAR_H, s->w, s->h - BAR_H, s->pixels);
    }

    /* Border: accent colour if focused, dimmed otherwise */
    unsigned int border_col = (handle == focused_handle) ? COL_BORDER
                                                          : FB_RGB(50, 50, 70);
    fb_draw_rect_outline(s->x, s->y, s->w, s->h, border_col);
}

void compositor_redraw(void) {
    if (!fb_get_width()) return;    /* framebuffer not initialised */

    /* Re-tile */
    apply_tiling_layout();

    /* Clear background */
    fb_clear(COL_BG);

    /* Render tiled surfaces first (by z-order) */
    for (unsigned int z = 0; z < surf_count; z++) {
        for (int i = 0; i < MAX_SURFACES; i++) {
            if ((surfaces[i].flags & SURF_FLAG_VISIBLE) &&
                !(surfaces[i].flags & SURF_FLAG_FLOATING) &&
                surfaces[i].z_order == z) {
                render_surface(i);
            }
        }
    }

    /* Then floating surfaces */
    for (unsigned int z = 0; z < surf_count; z++) {
        for (int i = 0; i < MAX_SURFACES; i++) {
            if ((surfaces[i].flags & (SURF_FLAG_VISIBLE | SURF_FLAG_FLOATING))
                    == (SURF_FLAG_VISIBLE | SURF_FLAG_FLOATING) &&
                surfaces[i].z_order == z) {
                render_surface(i);
            }
        }
    }

    fb_swap_buffers();
}

/* ---- Input handling ------------------------------------------------------ */

void compositor_handle_key(unsigned char scancode) {
    (void)scancode;
    /* TODO: translate scancode to key event and deliver to focused surface's
     * owner process via the message-passing layer (node_send_message). */
}

void compositor_focus(int handle) {
    if (handle < 0 || handle >= MAX_SURFACES) return;
    if (!(surfaces[handle].flags & SURF_FLAG_VISIBLE)) return;
    focused_handle = handle;
}

int compositor_get_focused(void) {
    return focused_handle;
}

void compositor_fill_surface(int handle, unsigned int colour) {
    if (handle < 0 || handle >= MAX_SURFACES) return;
    surface_t* s = &surfaces[handle];
    if (!(s->flags & SURF_FLAG_VISIBLE)) return;
    fb_draw_rect(s->x + 1, s->y + BAR_H, s->w - 2, s->h - BAR_H - 1, colour);
}
