#ifndef RAMFS_H
#define RAMFS_H

/* Simple in-memory filesystem used as initial VFS root.
 * Supports both RAM-backed and disk-backed files. */

#define RAMFS_MAX_FILES 16

void ramfs_init(void);

/* Lookup a file in the RAMFS root directory by name.
 * Returns 0 on success and stores a pointer to a VFS node in *out_node,
 * or -1 if not found.
 */
struct vfs_node;
int ramfs_lookup_root(const char* name, struct vfs_node** out_node);

/* Add a disk-backed file to the RAMFS root directory.
 * The file's data will be read from disk on demand.
 * Returns 0 on success, -1 if the table is full. */
int ramfs_add_disk_file(const char* name, unsigned int start_sector,
                        unsigned int size_bytes);

/* Add a RAM-backed file to the RAMFS root directory.
 * The data pointer must remain valid for the lifetime of the file.
 * Returns 0 on success, -1 if the table is full. */
int ramfs_add_ram_file(const char* name, const char* data,
                       unsigned int size_bytes);

#endif
