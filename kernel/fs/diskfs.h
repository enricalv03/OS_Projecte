#ifndef DISKFS_H
#define DISKFS_H

/* =============================================================================
 * SimpleFS -- A minimal read-only on-disk filesystem
 * =============================================================================
 *
 * On-disk layout (starting at DISKFS_START_SECTOR = 200):
 *
 *   Sector 200: SuperBlock
 *     [0..3]   magic     = 0x53465321  ("SFS!")
 *     [4..7]   version   = 1
 *     [8..11]  num_files
 *     [12..]   reserved (zero-padded to 512 bytes)
 *
 *   Sector 201+: File entries (48 bytes each, 10 per sector)
 *     [0..27]  name      (28 bytes, null-terminated)
 *     [28..31] start_sector  (absolute sector on disk)
 *     [32..35] size_bytes
 *     [36..39] type       (1=file, 2=dir)
 *     [40..47] reserved
 *
 *   Data sectors: file contents stored contiguously
 * =========================================================================== */

#define DISKFS_START_SECTOR  200
#define DISKFS_MAGIC         0x53465321  /* "SFS!" */
#define DISKFS_VERSION       1
#define DISKFS_MAX_FILES     16

/* On-disk superblock (512 bytes) */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int num_files;
    unsigned char reserved[500];
} __attribute__((packed)) diskfs_superblock_t;

/* On-disk file entry (48 bytes) */
typedef struct {
    char          name[28];
    unsigned int  start_sector;
    unsigned int  size_bytes;
    unsigned int  type;          /* 1=file, 2=dir */
    unsigned char reserved[8];
} __attribute__((packed)) diskfs_file_entry_t;

/* Initialize diskfs: reads the superblock and file table from disk,
 * and registers files with RAMFS so they appear in the VFS tree.
 * Call after block_init() and ramfs_init().
 * Returns 0 on success, -1 if no valid filesystem found. */
int diskfs_init(void);

#endif
