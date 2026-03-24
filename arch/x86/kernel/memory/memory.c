#include "memory.h"

/* Low-level PMM symbols from pmm.asm */
extern unsigned int pmm_get_free_pages(void);
extern unsigned int total_pages_value;

/* -------------------------------------------------------------------------
 * E820 memory map reader
 * ------------------------------------------------------------------------- */
void read_memory_map(memory_map_t* map) {
    map->count = *(unsigned int*)0x1FFC;
    memory_entry_t* src = (memory_entry_t*)0x2000;

    for (unsigned int i = 0; i < map->count && i < MAX_MEMORY_ENTRIES; i++) {
        map->entries[i] = src[i];
    }
}

/* -------------------------------------------------------------------------
 * Simple VGA text helpers for diagnostics (row/col, white on black).
 * These are intentionally minimal so they are easy to read and type.
 * ------------------------------------------------------------------------- */
#define VGA_TEXT_BASE 0xB8000
#define VGA_COLS      80

static void vga_write_str(unsigned int row, unsigned int col, const char* s) {
    volatile unsigned char* vga =
        (unsigned char*)(VGA_TEXT_BASE + 2 * (row * VGA_COLS + col));

    while (*s) {
        *vga++ = (unsigned char)*s++;
        *vga++ = 0x0F; /* white on black */
    }
}

static void vga_write_dec(unsigned int row, unsigned int col, unsigned int value) {
    char buf[16];
    unsigned int i = 0;

    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value > 0 && i < sizeof(buf) - 1) {
            buf[i++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    buf[i] = 0;

    /* Reverse in-place */
    for (unsigned int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }

    vga_write_str(row, col, buf);
}

/* -------------------------------------------------------------------------
 * pmm_dump_region_usage
 * -------------------------------------------------------------------------
 * Print a one-line summary of PMM free/used pages so we can manually verify
 * that it matches the calculations in memory_plan.txt. This relies on the
 * bitmap-based PMM (pmm_get_free_pages) and the total page count exported
 * from pmm.asm (total_pages_value).
 * ------------------------------------------------------------------------- */
void pmm_dump_region_usage(void) {
    unsigned int free_pages  = pmm_get_free_pages();
    unsigned int total_pages = total_pages_value;
    unsigned int used_pages  = total_pages - free_pages;

    /* Place this just below the memory region list (typically around row 9–10). */
    vga_write_str(10, 0, "PMM pages: free=");
    vga_write_dec(10, 17, free_pages);
    vga_write_str(10, 28, " used=");
    vga_write_dec(10, 35, used_pages);
}
