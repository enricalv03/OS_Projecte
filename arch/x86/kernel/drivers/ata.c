#include "ata.h"

/* =============================================================================
 * ATA PIO Mode Driver -- Implementation
 * =============================================================================
 * This is a minimal but correct ATA PIO driver for IDE disks.
 * It communicates directly with the ATA controller via x86 port I/O.
 *
 * Why PIO and not DMA?
 *   PIO is simpler (no bus mastering setup), works everywhere, and is
 *   sufficient for early OS development. DMA can be added later for speed.
 *
 * The QEMU default disk (-drive format=raw,file=disk.img) appears as
 * ATA primary master, which is exactly what this driver targets.
 * =========================================================================== */

/* ---- Inline port I/O wrappers ------------------------------------------- */

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Read 256 words (512 bytes = 1 sector) from the data port */
static inline void insw(unsigned short port, void* addr, unsigned int count) {
    unsigned short* buf = (unsigned short*)addr;
    for (unsigned int i = 0; i < count; i++) {
        buf[i] = inw(port);
    }
}

/* Write 256 words (512 bytes = 1 sector) to the data port */
static inline void outsw(unsigned short port, const void* addr, unsigned int count) {
    const unsigned short* buf = (const unsigned short*)addr;
    for (unsigned int i = 0; i < count; i++) {
        outw(port, buf[i]);
    }
}

/* ---- Internal helpers ---------------------------------------------------- */

/* 400ns delay by reading the alternate status register 4 times.
 * Each read takes ~100ns on standard hardware. */
static void ata_400ns_delay(void) {
    inb(ATA_PRIMARY_ALT_STATUS);
    inb(ATA_PRIMARY_ALT_STATUS);
    inb(ATA_PRIMARY_ALT_STATUS);
    inb(ATA_PRIMARY_ALT_STATUS);
}

/* Wait until BSY clears. Returns 0 on success, -1 on timeout. */
static int ata_wait_bsy(void) {
    /* Timeout after ~1 million iterations (~1 second on modern hardware) */
    for (int i = 0; i < 1000000; i++) {
        unsigned char status = inb(ATA_PRIMARY_STATUS);
        if (!(status & ATA_SR_BSY)) {
            return 0;
        }
    }
    return -1;  /* Timeout */
}

/* Wait until DRQ is set (data ready). Returns 0 on success, -1 on error. */
static int ata_wait_drq(void) {
    for (int i = 0; i < 1000000; i++) {
        unsigned char status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DF)  return -1;
        if (status & ATA_SR_DRQ) return 0;
    }
    return -1;  /* Timeout */
}

/* Track whether we've detected a drive */
static int ata_drive_present = 0;

/* ---- Public API ---------------------------------------------------------- */

int ata_init(void) {
    /* Software reset: set SRST bit, wait, clear it */
    outb(ATA_PRIMARY_DEV_CTRL, 0x04);  /* Set SRST */
    ata_400ns_delay();
    outb(ATA_PRIMARY_DEV_CTRL, 0x00);  /* Clear SRST */
    ata_400ns_delay();

    /* Select master drive */
    outb(ATA_PRIMARY_DRIVE_HEAD, ATA_MASTER);
    ata_400ns_delay();

    /* Check if a drive exists by reading the status register */
    unsigned char status = inb(ATA_PRIMARY_STATUS);
    if (status == 0xFF || status == 0x00) {
        /* No drive present (floating bus) */
        ata_drive_present = 0;
        return -1;
    }

    /* Wait for BSY to clear after reset */
    if (ata_wait_bsy() != 0) {
        ata_drive_present = 0;
        return -1;
    }

    /* Verify it's an ATA device (not ATAPI)
     * After IDENTIFY, LBA mid and high should be 0 for ATA */
    ata_drive_present = 1;
    return 0;
}

int ata_identify(unsigned short* buffer) {
    if (!ata_drive_present || !buffer) {
        return -1;
    }

    /* Select master drive */
    outb(ATA_PRIMARY_DRIVE_HEAD, ATA_MASTER);
    ata_400ns_delay();

    /* Clear sector count and LBA registers */
    outb(ATA_PRIMARY_SECT_COUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HIGH, 0);

    /* Send IDENTIFY command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
    ata_400ns_delay();

    /* Check if the drive responded */
    unsigned char status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        return -1;  /* Drive does not exist */
    }

    /* Wait for BSY to clear */
    if (ata_wait_bsy() != 0) {
        return -1;
    }

    /* Check that LBA mid/high are still 0 (otherwise it's ATAPI/SATA/etc) */
    if (inb(ATA_PRIMARY_LBA_MID) != 0 || inb(ATA_PRIMARY_LBA_HIGH) != 0) {
        return -1;  /* Not an ATA device */
    }

    /* Wait for DRQ or ERR */
    if (ata_wait_drq() != 0) {
        return -1;
    }

    /* Read 256 words of identification data */
    insw(ATA_PRIMARY_DATA, buffer, 256);
    return 0;
}

int ata_read_sectors(unsigned int lba, unsigned char count, void* buffer) {
    if (!ata_drive_present || !buffer || count == 0) {
        return -1;
    }

    /* Wait for drive to be ready */
    if (ata_wait_bsy() != 0) {
        return -1;
    }

    /* Select drive and set high 4 bits of LBA (LBA28 mode)
     * Bits 24-27 go in the low nibble of the drive/head register
     * Bit 6 = 1 for LBA mode */
    outb(ATA_PRIMARY_DRIVE_HEAD, ATA_MASTER | ((lba >> 24) & 0x0F));
    ata_400ns_delay();

    /* Set sector count */
    outb(ATA_PRIMARY_SECT_COUNT, count);

    /* Set LBA address (low 24 bits) */
    outb(ATA_PRIMARY_LBA_LOW,  (unsigned char)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID,  (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HIGH, (unsigned char)((lba >> 16) & 0xFF));

    /* Send READ SECTORS command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);

    /* Read each sector */
    unsigned short* buf = (unsigned short*)buffer;
    for (int i = 0; i < count; i++) {
        /* Wait for data to be ready */
        if (ata_wait_drq() != 0) {
            return -1;
        }

        /* Read 256 words (512 bytes = 1 sector) */
        insw(ATA_PRIMARY_DATA, buf, 256);
        buf += 256;  /* Advance by 256 words = 512 bytes */
    }

    return 0;
}

int ata_write_sectors(unsigned int lba, unsigned char count, const void* buffer) {
    if (!ata_drive_present || !buffer || count == 0) {
        return -1;
    }

    /* Wait for drive to be ready */
    if (ata_wait_bsy() != 0) {
        return -1;
    }

    /* Select drive and set LBA28 high nibble */
    outb(ATA_PRIMARY_DRIVE_HEAD, ATA_MASTER | ((lba >> 24) & 0x0F));
    ata_400ns_delay();

    /* Set sector count and LBA */
    outb(ATA_PRIMARY_SECT_COUNT, count);
    outb(ATA_PRIMARY_LBA_LOW,  (unsigned char)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID,  (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HIGH, (unsigned char)((lba >> 16) & 0xFF));

    /* Send WRITE SECTORS command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);

    /* Write each sector */
    const unsigned short* buf = (const unsigned short*)buffer;
    for (int i = 0; i < count; i++) {
        /* Wait for drive to accept data */
        if (ata_wait_drq() != 0) {
            return -1;
        }

        /* Write 256 words (512 bytes = 1 sector) */
        outsw(ATA_PRIMARY_DATA, buf, 256);
        buf += 256;  /* Advance by 256 words = 512 bytes */
    }

    /* Flush cache after writing to ensure data hits the platter */
    return ata_flush_cache();
}

int ata_flush_cache(void) {
    /* Select master drive */
    outb(ATA_PRIMARY_DRIVE_HEAD, ATA_MASTER);
    ata_400ns_delay();

    /* Send CACHE FLUSH command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);

    /* Wait for completion */
    return ata_wait_bsy();
}
