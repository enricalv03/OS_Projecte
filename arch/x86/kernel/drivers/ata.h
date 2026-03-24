#ifndef ATA_H
#define ATA_H

/* =============================================================================
 * ATA PIO Mode Driver
 * =============================================================================
 * Drives IDE/ATA hard disks using Programmed I/O (PIO) mode.
 * PIO is slow but simple and works on every x86 machine including QEMU.
 *
 * This driver supports:
 *   - Device identification (IDENTIFY command)
 *   - LBA28 read/write (up to 128 GB)
 *   - Cache flush
 *
 * Port layout for primary ATA controller:
 *   0x1F0  Data register (16-bit)
 *   0x1F1  Error / Features
 *   0x1F2  Sector Count
 *   0x1F3  LBA Low  (bits 0-7)
 *   0x1F4  LBA Mid  (bits 8-15)
 *   0x1F5  LBA High (bits 16-23)
 *   0x1F6  Drive/Head (bits 24-27 of LBA + drive select)
 *   0x1F7  Status (read) / Command (write)
 *   0x3F6  Alternate Status / Device Control
 * =========================================================================== */

/* ATA I/O Ports (Primary Controller) */
#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_ERROR       0x1F1
#define ATA_PRIMARY_FEATURES    0x1F1
#define ATA_PRIMARY_SECT_COUNT  0x1F2
#define ATA_PRIMARY_LBA_LOW     0x1F3
#define ATA_PRIMARY_LBA_MID     0x1F4
#define ATA_PRIMARY_LBA_HIGH    0x1F5
#define ATA_PRIMARY_DRIVE_HEAD  0x1F6
#define ATA_PRIMARY_STATUS      0x1F7
#define ATA_PRIMARY_COMMAND     0x1F7
#define ATA_PRIMARY_ALT_STATUS  0x3F6
#define ATA_PRIMARY_DEV_CTRL    0x3F6

/* ATA Status Register Bits */
#define ATA_SR_BSY    0x80   /* Busy */
#define ATA_SR_DRDY   0x40   /* Device Ready */
#define ATA_SR_DF     0x20   /* Device Fault */
#define ATA_SR_DSC    0x10   /* Drive Seek Complete */
#define ATA_SR_DRQ    0x08   /* Data Request Ready */
#define ATA_SR_CORR   0x04   /* Corrected Data */
#define ATA_SR_IDX    0x02   /* Index */
#define ATA_SR_ERR    0x01   /* Error */

/* ATA Commands */
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_IDENTIFY        0xEC

/* Drive selection */
#define ATA_MASTER              0xE0   /* Master drive, LBA mode */
#define ATA_SLAVE               0xF0   /* Slave drive, LBA mode */

/* Initialize the ATA driver. Probes for primary master drive.
 * Returns: 0 on success, -1 if no drive found */
int ata_init(void);

/* Send IDENTIFY command to get drive information.
 * buffer: pointer to 256 unsigned shorts (512 bytes)
 * Returns: 0 on success, -1 on failure */
int ata_identify(unsigned short* buffer);

/* Read sectors using PIO mode (LBA28).
 * lba:    starting logical block address
 * count:  number of sectors to read (1-255)
 * buffer: destination buffer (must be at least count * 512 bytes)
 * Returns: 0 on success, -1 on error */
int ata_read_sectors(unsigned int lba, unsigned char count, void* buffer);

/* Write sectors using PIO mode (LBA28).
 * lba:    starting logical block address
 * count:  number of sectors to write (1-255)
 * buffer: source data (must be at least count * 512 bytes)
 * Returns: 0 on success, -1 on error */
int ata_write_sectors(unsigned int lba, unsigned char count, const void* buffer);

/* Flush the drive's write cache to ensure data is on disk.
 * Returns: 0 on success, -1 on error */
int ata_flush_cache(void);

#endif
