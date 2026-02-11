#include "vfs.h"
#include "ramfs.h"
#include "../drivers/block.h"

/* --------------------------------------------------------------------------
 * Internal RAMFS structures
 * -------------------------------------------------------------------------- */

typedef struct ramfs_node {
  vfs_node_type_t type;
  char            name[32];
  const char*     data;           /* RAM-backed data pointer (NULL for disk files) */
  unsigned int    size;
  unsigned int    disk_sector;    /* >0 = disk-backed file (starting sector) */
} ramfs_node_t;

/* Static storage for up to RAMFS_MAX_FILES + root */
static ramfs_node_t  ramfs_nodes[RAMFS_MAX_FILES];
static ramfs_node_t* ramfs_children[RAMFS_MAX_FILES];
static unsigned int  ramfs_file_count = 0;

static ramfs_node_t  ramfs_root_storage;

static vfs_node_t ramfs_vnodes[RAMFS_MAX_FILES];
static vfs_node_t ramfs_root_vnode;

/* --------------------------------------------------------------------------
 * String helpers (no libc)
 * -------------------------------------------------------------------------- */

static int ramfs_strcmp(const char* a, const char* b) {
  if (!a || !b) return (a == b) ? 0 : 1;
  while (*a || *b) {
    unsigned char ca = (unsigned char)*a;
    unsigned char cb = (unsigned char)*b;
    if (ca != cb) return (int)ca - (int)cb;
    if (ca == 0) break;
    a++; b++;
  }
  return 0;
}

static void ramfs_strcpy(char* dst, const char* src, unsigned int max) {
  unsigned int i = 0;
  while (src[i] != 0 && i < max - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = 0;
}

/* --------------------------------------------------------------------------
 * VFS operation callbacks
 * -------------------------------------------------------------------------- */

static int ramfs_read(vfs_node_t* node,
                      unsigned int offset,
                      unsigned int size,
                      void* buffer) {
  if (!node || !buffer) return -1;

  ramfs_node_t* rnode = (ramfs_node_t*)node->fs_private;
  if (!rnode || rnode->type != VFS_NODE_FILE) return -1;
  if (offset >= rnode->size) return 0;
  if (offset + size > rnode->size) {
    size = rnode->size - offset;
  }

  if (rnode->disk_sector > 0) {
    /* ----- Disk-backed file read ----- */
    /* Calculate which sector(s) to read */
    unsigned int byte_offset = offset;
    unsigned int start_sect = rnode->disk_sector + (byte_offset / 512);
    unsigned int sect_offset = byte_offset % 512;
    unsigned int bytes_read = 0;
    unsigned char sect_buf[512];
    char* dst = (char*)buffer;

    while (bytes_read < size) {
      if (block_read("ata0", start_sect, 1, sect_buf) != 0) {
        return (int)bytes_read;  /* partial read */
      }
      unsigned int chunk = 512 - sect_offset;
      if (chunk > size - bytes_read) {
        chunk = size - bytes_read;
      }
      for (unsigned int i = 0; i < chunk; i++) {
        dst[bytes_read + i] = (char)sect_buf[sect_offset + i];
      }
      bytes_read += chunk;
      sect_offset = 0;
      start_sect++;
    }
    return (int)bytes_read;
  } else {
    /* ----- RAM-backed file read ----- */
    if (!rnode->data) return -1;
    const char* src = rnode->data + offset;
    char* dst = (char*)buffer;
    for (unsigned int i = 0; i < size; i++) {
      dst[i] = src[i];
    }
    return (int)size;
  }
}

static int ramfs_readdir(vfs_node_t* node,
                         unsigned int index,
                         vfs_dir_entry_t* out) {
  if (!node || !out) return -1;
  (void)node; /* root is implicit */

  if (index >= ramfs_file_count) {
    return -1;  /* No more entries */
  }

  ramfs_node_t* child = ramfs_children[index];
  ramfs_strcpy(out->name, child->name, sizeof(out->name));
  out->type = child->type;
  return 0;
}

/* Forward declaration */
static vfs_node_t* ramfs_vfs_lookup(vfs_node_t* dir, const char* name);

static vfs_ops_t ramfs_dir_ops = {
  0,               /* open    */
  0,               /* close   */
  0,               /* read    */
  0,               /* write   */
  ramfs_readdir,   /* readdir */
  ramfs_vfs_lookup /* lookup  */
};

static vfs_ops_t ramfs_file_ops = {
  0,         /* open    */
  0,         /* close   */
  ramfs_read,
  0,         /* write   */
  0,         /* readdir */
  0          /* lookup  */
};

/* --------------------------------------------------------------------------
 * VFS lookup callback for RAMFS directories
 * -------------------------------------------------------------------------- */

static vfs_node_t* ramfs_vfs_lookup(vfs_node_t* dir, const char* name) {
  if (!dir || !name) return 0;
  (void)dir;

  for (unsigned int i = 0; i < ramfs_file_count; i++) {
    if (ramfs_strcmp(ramfs_children[i]->name, name) == 0) {
      return &ramfs_vnodes[i];
    }
  }
  return 0;
}

/* --------------------------------------------------------------------------
 * Internal: add a file node to the RAMFS
 * -------------------------------------------------------------------------- */

static int ramfs_add_node(const char* name, const char* data,
                          unsigned int size, unsigned int disk_sector) {
  if (ramfs_file_count >= RAMFS_MAX_FILES) return -1;

  unsigned int idx = ramfs_file_count;
  ramfs_node_t* rn = &ramfs_nodes[idx];
  rn->type = VFS_NODE_FILE;
  ramfs_strcpy(rn->name, name, sizeof(rn->name));
  rn->data = data;
  rn->size = size;
  rn->disk_sector = disk_sector;

  ramfs_children[idx] = rn;

  /* Set up VFS node */
  vfs_node_t* vn = &ramfs_vnodes[idx];
  ramfs_strcpy(vn->name, name, sizeof(vn->name));
  vn->type = VFS_NODE_FILE;
  vn->size = size;
  vn->fs_private = rn;
  vn->ops = &ramfs_file_ops;
  vn->parent = &ramfs_root_vnode;

  ramfs_file_count++;
  return 0;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

static const char readme_text[] =
  "MyOS ramfs root\n"
  "This is a simple in-memory filesystem used for testing.\n";

static const char about_text[] =
  "About MyOS\n"
  "A learning operating system with multilingual commands and VFS.\n";

void ramfs_init(void) {
  ramfs_file_count = 0;

  /* Set up root node */
  ramfs_root_storage.type = VFS_NODE_DIR;
  ramfs_strcpy(ramfs_root_storage.name, "", sizeof(ramfs_root_storage.name));
  ramfs_root_storage.data = 0;
  ramfs_root_storage.size = 0;
  ramfs_root_storage.disk_sector = 0;

  /* Set up root VFS node */
  for (unsigned int i = 0; i < sizeof(ramfs_root_vnode.name); i++) {
    ramfs_root_vnode.name[i] = 0;
  }
  ramfs_root_vnode.type = VFS_NODE_DIR;
  ramfs_root_vnode.size = 0;
  ramfs_root_vnode.fs_private = &ramfs_root_storage;
  ramfs_root_vnode.ops = &ramfs_dir_ops;
  ramfs_root_vnode.parent = 0;

  /* Add built-in RAM files */
  ramfs_add_node("readme.txt", readme_text,
                 (unsigned int)sizeof(readme_text) - 1, 0);
  ramfs_add_node("about.txt", about_text,
                 (unsigned int)sizeof(about_text) - 1, 0);

  /* Mount as VFS root */
  vfs_mount_root(&ramfs_root_vnode);
}

int ramfs_add_disk_file(const char* name, unsigned int start_sector,
                        unsigned int size_bytes) {
  return ramfs_add_node(name, 0, size_bytes, start_sector);
}

int ramfs_add_ram_file(const char* name, const char* data,
                       unsigned int size_bytes) {
  return ramfs_add_node(name, data, size_bytes, 0);
}

/* Lookup a file in the RAMFS root by name. */
int ramfs_lookup_root(const char* name, vfs_node_t** out_node) {
  if (!name || !out_node) return -1;

  for (unsigned int i = 0; i < ramfs_file_count; i++) {
    if (ramfs_strcmp(ramfs_children[i]->name, name) == 0) {
      *out_node = &ramfs_vnodes[i];
      return 0;
    }
  }
  return -1;
}
