#include "block.h"

/* =============================================================================
 * ATA Disk Test - Hex dump of a disk sector via the console
 * =============================================================================
 * Reads one sector (512 bytes) and displays the first 256 bytes as a
 * formatted hex dump integrated with the kernel console output.
 *
 * Output format per line:
 *   OFFSET: HH HH HH HH HH HH HH HH  HH HH HH HH HH HH HH HH |ASCII...........|
 * =========================================================================== */

/* Console cursor and scroll function from kernel.asm / screen.asm */
extern unsigned int cons_cursor;
extern void scroll_up_one_row(void);

/* ---------------------------------------------------------------------------
 * Internal VGA helpers - write directly to VGA text memory via cons_cursor
 * --------------------------------------------------------------------------- */

static void console_putchar(char c, unsigned char attr) {
    volatile unsigned char* vga = (volatile unsigned char*)(unsigned int)cons_cursor;
    *vga = (unsigned char)c;
    *(vga + 1) = attr;
    cons_cursor += 2;
}

static void console_newline(void) {
    unsigned int offset = cons_cursor - 0xb8000;
    unsigned int row = offset / 160;
    row++;
    if (row >= 25) {
        scroll_up_one_row();
        row = 24;
    }
    cons_cursor = 0xb8000 + row * 160;
}

static void console_put_hex_nibble(unsigned char nibble, unsigned char attr) {
    char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    console_putchar(c, attr);
}

static void console_put_hex_byte(unsigned char byte, unsigned char attr) {
    console_put_hex_nibble((byte >> 4) & 0x0F, attr);
    console_put_hex_nibble(byte & 0x0F, attr);
}

static void console_put_hex32(unsigned int val, unsigned char attr) {
    for (int i = 28; i >= 0; i -= 4) {
        console_put_hex_nibble((val >> i) & 0x0F, attr);
    }
}

static void console_puts(const char* s, unsigned char attr) {
    while (*s) {
        console_putchar(*s++, attr);
    }
}

/* ---------------------------------------------------------------------------
 * Public API - called from commands.asm via CDECL
 * ---------------------------------------------------------------------------
 * int ata_test_read_sector(unsigned int sector)
 *   Returns: 0 on success, -1 on read error
 * --------------------------------------------------------------------------- */
int ata_test_read_sector(unsigned int sector) {
    unsigned char buffer[512];

    /* Read one sector via the block device layer */
    if (block_read("ata0", sector, 1, buffer) != 0) {
        return -1; /* caller prints error message */
    }

    /* Header */
    console_puts("--- Sector 0x", 0x0B);
    console_put_hex32(sector, 0x0B);
    console_puts(" ---", 0x0B);
    console_newline();

    /* Print 256 bytes (16 rows of 16 bytes) */
    for (unsigned int i = 0; i < 256; i++) {
        if (i % 16 == 0) {
            /* Offset column */
            console_put_hex32(sector * 512 + i, 0x07);
            console_puts(": ", 0x07);
        }

        /* Hex byte */
        console_put_hex_byte(buffer[i], 0x0F);
        console_putchar(' ', 0x07);

        /* Extra space at the midpoint for readability */
        if (i % 16 == 7) {
            console_putchar(' ', 0x07);
        }

        /* End of row: print ASCII representation */
        if ((i + 1) % 16 == 0) {
            console_putchar('|', 0x08);
            for (unsigned int j = i - 15; j <= i; j++) {
                char c = (char)buffer[j];
                if (c < 32 || c > 126) c = '.';
                console_putchar(c, 0x0A);
            }
            console_putchar('|', 0x08);
            console_newline();
        }
    }

    return 0;
}
