#ifndef DEVFS_H
#define DEVFS_H

/* =============================================================================
 * devfs — pseudo-device virtual filesystem
 * =============================================================================
 * Provides special character device files:
 *   /dev/null    — read returns 0 bytes; writes are discarded.
 *   /dev/zero    — read returns an infinite stream of zero bytes.
 *   /dev/console — read/write is wired to the VGA text console.
 *   /dev/random  — read returns pseudo-random bytes (linear congruential).
 *
 * The /dev directory is created as a sub-directory of the RAMFS root and
 * the device nodes are installed there as VFS files backed by devfs ops.
 * =========================================================================== */

/* Initialise devfs and mount the device nodes under /dev.
 * Must be called after ramfs_init() and vfs_mount_root(). */
void devfs_init(void);

#endif
