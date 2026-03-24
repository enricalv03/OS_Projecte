#include "diskfs.h"
#include "ramfs.h"
#include "drivers/block.h"

/* =============================================================================
 * diskfs_init -- Probe disk for a SimpleFS partition and register files
 * =============================================================================
 * Reads the superblock from DISKFS_START_SECTOR.  If the magic matches,
 * reads the file table and registers each file with RAMFS as a disk-backed
 * entry so it appears in `ls` and is readable with `cat`.
 *
 * Returns 0 on success, -1 if no valid filesystem was found.
 * =========================================================================== */

int diskfs_init(void) {
    static unsigned char sector_buf[512];

    /* Ensure block device exists (block_init may skip if no ATA drive) */
    if (block_device_get("ata0") == 0) {
        return -1;
    }

    /* Step 1: Read superblock from sector 200 */
    if (block_read("ata0", DISKFS_START_SECTOR, 1, sector_buf) != 0) {
        return -1;  /* disk read failed */
    }

    diskfs_superblock_t* sb = (diskfs_superblock_t*)sector_buf;

    /* Validate magic number */
    if (sb->magic != DISKFS_MAGIC) {
        return -1;  /* no SimpleFS found -- that's OK */
    }

    if (sb->version != DISKFS_VERSION) {
        return -1;  /* unsupported version */
    }

    unsigned int num_files = sb->num_files;
    if (num_files == 0 || num_files > DISKFS_MAX_FILES) {
        return -1;
    }

    /* Step 2: Read file entry table (sector 201+)
     * Each entry is 48 bytes; 10 fit per sector.
     * We may need multiple sectors for > 10 files. */
    unsigned int entries_per_sector = 512 / sizeof(diskfs_file_entry_t);
    unsigned int sectors_needed = (num_files + entries_per_sector - 1)
                                  / entries_per_sector;

    unsigned int files_registered = 0;

    for (unsigned int s = 0; s < sectors_needed && files_registered < num_files; s++) {
        if (block_read("ata0", DISKFS_START_SECTOR + 1 + s, 1, sector_buf) != 0) {
            break;  /* read failed, stop but keep what we got */
        }

        diskfs_file_entry_t* entries = (diskfs_file_entry_t*)sector_buf;
        unsigned int count_in_sector = entries_per_sector;
        if (files_registered + count_in_sector > num_files) {
            count_in_sector = num_files - files_registered;
        }

        for (unsigned int i = 0; i < count_in_sector; i++) {
            diskfs_file_entry_t* fe = &entries[i];

            /* Sanity check: name must be non-empty */
            if (fe->name[0] == 0) continue;

            /* Only register files (type 1), skip directories for now */
            if (fe->type != 1) continue;

            /* Ensure name is null-terminated */
            fe->name[27] = 0;

            /* Register with RAMFS as a disk-backed file */
            ramfs_add_disk_file(fe->name, fe->start_sector, fe->size_bytes);
            files_registered++;
        }
    }

    return 0;
}
