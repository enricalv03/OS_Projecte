#ifndef RAMFS_H
#define RAMFS_H

/* Simple in-memory filesystem used as initial VFS root.
 * Supports both RAM-backed and disk-backed files. */

#define RAMFS_MAX_FILES 48

void ramfs_init(void);

/* Return the VFS node for the ramfs root (for re-mount / recovery). */
struct vfs_node* ramfs_get_vfs_root(void);

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


int ramfs_add_ram_file_under(struct vfs_node* dir, const char* name, const char* data, unsigned int size_bytes);

int ramfs_add_ram_file_at_path(const char* path, const char* data, unsigned int size_bytes);

/* Create a directory at the given path (e.g. "/home/newuser" or "sistem/config").
 * Parent must exist and be a directory. Returns 0 on success, -1 on error (parent
 * not found, not a dir, or filesystem full), -2 if a node with that name already exists. */
int ramfs_mkdir_at_path(const char* path);

/* Copy a file from src_path to dest_path. Both must be absolute paths.
 * Allocates heap memory for the copy (persistent).
 * Returns 0 on success, -1 src not found/not a file, -2 dest exists, -3 out of memory. */
int ramfs_copy_file(const char* src_path, const char* dest_path);

/* Move/rename a node (file or directory). Relinks the parent pointer and
 * renames — no data copy. Works for both files and directories.
 * Returns 0 on success, -1 src not found / dest parent invalid / can't move root,
 *         -2 dest already exists. */
int ramfs_move_node(const char* src_path, const char* dest_path);

/* Remove a file or empty directory. Returns 0 on success, -1 if root, -2 if dir not empty. */
int ramfs_remove_node(struct vfs_node* node);

//int ramfs_remove_node_recursive(struct vfs_node* node);

/* Recursive search: find all nodes named `name` under `start_path`.
 * If start_path is "/" or NULL, searches the entire filesystem.
 * Returns the number of matches (up to 16). Use ramfs_find_get_result(i)
 * to retrieve the full path of each match. */
int ramfs_find(const char* start_path, const char* name);
const char* ramfs_find_get_result(int index);

#endif
