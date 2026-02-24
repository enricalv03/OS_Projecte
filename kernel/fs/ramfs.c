#include "vfs.h"
#include "ramfs.h"
#include "../drivers/block.h"
#include "../memory/heap.h"

/* --------------------------------------------------------------------------
 * Internal RAMFS structures
 * -------------------------------------------------------------------------- */

typedef struct ramfs_node {
  vfs_node_type_t    type;
  char               name[32];
  const char*        data;           /* RAM-backed data pointer (NULL for disk files) */
  unsigned int       size;
  unsigned int       disk_sector;    /* >0 = disk-backed file (starting sector) */
  struct ramfs_node* parent;         /* NULL only for root; else parent directory */
} ramfs_node_t;

/* Static storage for up to RAMFS_MAX_FILES entries (files + directories) */
static ramfs_node_t  ramfs_nodes[RAMFS_MAX_FILES];
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
 * Get VFS node for a ramfs node (for setting parent pointers)
 * -------------------------------------------------------------------------- */
static vfs_node_t* ramfs_get_vnode_for(ramfs_node_t* rn) {
  if (rn == &ramfs_root_storage)
    return &ramfs_root_vnode;
  for (unsigned int i = 0; i < ramfs_file_count; i++) {
    if (&ramfs_nodes[i] == rn)
      return &ramfs_vnodes[i];
  }
  return 0;
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
    unsigned int byte_offset = offset;
    unsigned int start_sect = rnode->disk_sector + (byte_offset / 512);
    unsigned int sect_offset = byte_offset % 512;
    unsigned int bytes_read = 0;
    unsigned char sect_buf[512];
    char* dst = (char*)buffer;

    while (bytes_read < size) {
      if (block_read("ata0", start_sect, 1, sect_buf) != 0) {
        return (int)bytes_read;
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

  ramfs_node_t* dir = (ramfs_node_t*)node->fs_private;
  if (!dir || dir->type != VFS_NODE_DIR) return -1;

  unsigned int count = 0;
  for (unsigned int i = 0; i < ramfs_file_count; i++) {
    if (ramfs_nodes[i].parent != dir)
      continue;
    if (ramfs_nodes[i].name[0] == 0)
      continue; /* deleted node */
    if (count == index) {
      ramfs_strcpy(out->name, ramfs_nodes[i].name, sizeof(out->name));
      out->type = ramfs_nodes[i].type;
      return 0;
    }
    count++;
  }
  return -1;  /* no more entries */
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
 * VFS lookup: find a child of dir by name
 * -------------------------------------------------------------------------- */
static vfs_node_t* ramfs_vfs_lookup(vfs_node_t* dir, const char* name) {
  if (!dir || !name) return 0;

  ramfs_node_t* dir_rn = (ramfs_node_t*)dir->fs_private;
  if (!dir_rn || dir_rn->type != VFS_NODE_DIR) return 0;

  for (unsigned int i = 0; i < ramfs_file_count; i++) {
    if (ramfs_nodes[i].parent != dir_rn)
      continue;
    if (ramfs_nodes[i].name[0] == 0)
      continue; /* deleted node */
    if (ramfs_strcmp(ramfs_nodes[i].name, name) == 0)
      return &ramfs_vnodes[i];
  }
  return 0;
}

/* --------------------------------------------------------------------------
 * Internal: add a file node under parent
 * -------------------------------------------------------------------------- */
static int ramfs_add_node(ramfs_node_t* parent, const char* name, const char* data,
                          unsigned int size, unsigned int disk_sector) {
  if (ramfs_file_count >= RAMFS_MAX_FILES) return -1;

  unsigned int idx = ramfs_file_count;
  ramfs_node_t* rn = &ramfs_nodes[idx];
  rn->type = VFS_NODE_FILE;
  ramfs_strcpy(rn->name, name, sizeof(rn->name));
  rn->data = data;
  rn->size = size;
  rn->disk_sector = disk_sector;
  rn->parent = parent;

  vfs_node_t* vn = &ramfs_vnodes[idx];
  ramfs_strcpy(vn->name, name, sizeof(vn->name));
  vn->type = VFS_NODE_FILE;
  vn->size = size;
  vn->fs_private = rn;
  vn->ops = &ramfs_file_ops;
  vn->parent = ramfs_get_vnode_for(parent);

  ramfs_file_count++;
  return 0;
}

/* --------------------------------------------------------------------------
 * Add a directory under parent. Returns pointer to new ramfs_node for
 * use as parent of future children, or 0 on failure.
 * -------------------------------------------------------------------------- */
static ramfs_node_t* ramfs_add_dir(ramfs_node_t* parent, const char* name) {
  if (ramfs_file_count >= RAMFS_MAX_FILES) return 0;

  unsigned int idx = ramfs_file_count;
  ramfs_node_t* rn = &ramfs_nodes[idx];
  rn->type = VFS_NODE_DIR;
  ramfs_strcpy(rn->name, name, sizeof(rn->name));
  rn->data = 0;
  rn->size = 0;
  rn->disk_sector = 0;
  rn->parent = parent;

  vfs_node_t* vn = &ramfs_vnodes[idx];
  ramfs_strcpy(vn->name, name, sizeof(vn->name));
  vn->type = VFS_NODE_DIR;
  vn->size = 0;
  vn->fs_private = rn;
  vn->ops = &ramfs_dir_ops;
  vn->parent = ramfs_get_vnode_for(parent);

  ramfs_file_count++;
  return rn;
}

/* --------------------------------------------------------------------------
 * Public API (add file under root)
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
  ramfs_root_storage.parent = 0;

  /* Set up root VFS node */
  for (unsigned int i = 0; i < sizeof(ramfs_root_vnode.name); i++) {
    ramfs_root_vnode.name[i] = 0;
  }
  ramfs_root_vnode.type = VFS_NODE_DIR;
  ramfs_root_vnode.size = 0;
  ramfs_root_vnode.fs_private = &ramfs_root_storage;
  ramfs_root_vnode.ops = &ramfs_dir_ops;
  ramfs_root_vnode.parent = 0;

  /* Add built-in RAM files under root */
  ramfs_add_node(&ramfs_root_storage, "readme.txt", readme_text,
                 (unsigned int)sizeof(readme_text) - 1, 0);
  ramfs_add_node(&ramfs_root_storage, "about.txt", about_text,
                 (unsigned int)sizeof(about_text) - 1, 0);

  /* Create directory tree: /sistem, /admin, /home, /bin, /tmp, /logs, /boot */
  ramfs_add_dir(&ramfs_root_storage, "sistem");
  ramfs_add_dir(&ramfs_root_storage, "admin");
  ramfs_node_t* home_dir = ramfs_add_dir(&ramfs_root_storage, "home");
  if (home_dir) {
    ramfs_add_dir(home_dir, "normal");   /* /home/normal - normal user profile */
    ramfs_add_dir(home_dir, "hacking");  /* /home/hacking - technical user profile */
  }
  ramfs_add_dir(&ramfs_root_storage, "bin");
  ramfs_add_dir(&ramfs_root_storage, "tmp");
  ramfs_add_dir(&ramfs_root_storage, "logs");
  ramfs_add_dir(&ramfs_root_storage, "boot");

  /* Mount as VFS root */
  vfs_mount_root(&ramfs_root_vnode);
}

vfs_node_t* ramfs_get_vfs_root(void) {
  return &ramfs_root_vnode;
}

int ramfs_add_disk_file(const char* name, unsigned int start_sector,
                        unsigned int size_bytes) {
  return ramfs_add_node(&ramfs_root_storage, name, 0, size_bytes, start_sector);
}

int ramfs_add_ram_file(const char* name, const char* data,
                       unsigned int size_bytes) {
  return ramfs_add_node(&ramfs_root_storage, name, data, size_bytes, 0);
}

/* Lookup a file in the RAMFS root by name (backward compatibility). */
int ramfs_lookup_root(const char* name, vfs_node_t** out_node) {
  if (!name || !out_node) return -1;

  for (unsigned int i = 0; i < ramfs_file_count; i++) {
    if (ramfs_nodes[i].parent != &ramfs_root_storage)
      continue;
    if (ramfs_strcmp(ramfs_nodes[i].name, name) == 0) {
      *out_node = &ramfs_vnodes[i];
      return 0;
    }
  }
  return -1;
}

int ramfs_add_ram_file_under(struct vfs_node* dir, const char* name, const char* data, unsigned int size_bytes) {
  if (!dir || dir->type != VFS_NODE_DIR || !name || !data)
  {
    return -1;
  }
  ramfs_node_t* parent = (ramfs_node_t*)dir->fs_private;
  if (!parent || parent->type != VFS_NODE_DIR)
  {
    return -1;
  }
  
  return ramfs_add_node(parent, name, data, size_bytes, 0);
}

static int path_strlen(const char* s) {
  int n = 0;
  while (s[n] != 0)
  {
    n++;
  }
  return n;
}

int ramfs_add_ram_file_at_path(const char* path, const char* data, unsigned int size_bytes) {
  if (!path || path[0] == 0|| !data)
  {
    return -1;
  }
  
  char path_buf[64];
  int plen = path_strlen(path);
  if (plen >= (int)sizeof(path_buf))
  {
    return -1;
  }
  
  const char* use_path = path;
  if (path[0] != '/')
  {
    path_buf[0] = '/';
    for (int i = 0; i < plen; i++)
    {
      path_buf[i + 1] = path[i];
    }
    path_buf[plen + 1] = '\0';
    use_path = path_buf;
    plen++;
  }

  int last_slash = -1;
  for (int i = 0; use_path[i] != 0; i++)
  {
    if (use_path[i] == '/')
    {
      last_slash = i;
    }
  }
  
  char parent_buf[64];
  char base_buf[32];
  if (last_slash < 0)
  {
    parent_buf[0] = '/';
    parent_buf[1] = 0;
    int j = 0;
    while (use_path[j] != 0 && j < 31)
    {
      base_buf[j] = use_path[j];
      j++;
    }
    base_buf[j] = 0;
  } else {
    int i;
    for (i = 0; i < last_slash && i < 63; i++)
    {
      parent_buf[i] = use_path[i];
    }
    parent_buf[i] = 0;
    if (last_slash + 1 >= plen)
    {
      return -1;
    }
    int j = 0;
    for (i = last_slash + 1; use_path[i] != 0 && j < 31; i++, j++)
    {
      base_buf[j] = use_path[i];
    }
    base_buf[j] = 0;
  }
  if (base_buf[0] == 0)
  {
    return -1;
  }
  
  vfs_node_t* dir_node = vfs_lookup(parent_buf);
  if (!dir_node || dir_node->type != VFS_NODE_DIR)
  {
    return -1;
  }
  
  return ramfs_add_ram_file_under(dir_node, base_buf, data, size_bytes);
}

/* --------------------------------------------------------------------------
 * Create a directory under a VFS directory node (used by ramfs_mkdir_at_path).
 * Returns 0 on success, -1 on failure (invalid dir or filesystem full).
 * -------------------------------------------------------------------------- */
static int ramfs_add_dir_under(struct vfs_node* dir, const char* name) {
  if (!dir || dir->type != VFS_NODE_DIR || !name || name[0] == 0)
    return -1;
  ramfs_node_t* parent = (ramfs_node_t*)dir->fs_private;
  if (!parent || parent->type != VFS_NODE_DIR)
    return -1;
  if (ramfs_file_count >= RAMFS_MAX_FILES)
    return -1;
  ramfs_add_dir(parent, name);
  return 0;
}

/* --------------------------------------------------------------------------
 * Create directory at path. Path can be "/sistem/newdir" or "sistem/newdir".
 * Returns: 0 success, -1 parent invalid or full, -2 already exists.
 * -------------------------------------------------------------------------- */
int ramfs_mkdir_at_path(const char* path) {
  if (!path || path[0] == 0)
    return -1;

  char path_buf[64];
  int plen = path_strlen(path);
  if (plen >= (int)sizeof(path_buf))
    return -1;

  const char* use_path = path;
  if (path[0] != '/') {
    path_buf[0] = '/';
    for (int i = 0; i < plen; i++)
      path_buf[i + 1] = path[i];
    path_buf[plen + 1] = '\0';
    use_path = path_buf;
    plen++;
  }

  int last_slash = -1;
  for (int i = 0; use_path[i] != 0; i++) {
    if (use_path[i] == '/')
      last_slash = i;
  }

  char parent_buf[64];
  char base_buf[32];
  if (last_slash < 0) {
    parent_buf[0] = '/';
    parent_buf[1] = 0;
    int j = 0;
    while (use_path[j] != 0 && j < 31) {
      base_buf[j] = use_path[j];
      j++;
    }
    base_buf[j] = 0;
  } else {
    int i;
    for (i = 0; i < last_slash && i < 63; i++)
      parent_buf[i] = use_path[i];
    parent_buf[i] = 0;
    if (last_slash + 1 >= plen)
      return -1;
    int j = 0;
    for (i = last_slash + 1; use_path[i] != 0 && j < 31; i++, j++)
      base_buf[j] = use_path[i];
    base_buf[j] = 0;
  }
  if (base_buf[0] == 0)
    return -1;

  /* Already exists? */
  vfs_node_t* existing = vfs_lookup(use_path);
  if (existing)
    return -2;

  vfs_node_t* dir_node = vfs_lookup(parent_buf);
  if (!dir_node || dir_node->type != VFS_NODE_DIR)
    return -1;

  return ramfs_add_dir_under(dir_node, base_buf);
}

/* --------------------------------------------------------------------------
 * Copy a file from src_path to dest_path.
 * Allocates heap memory for the copied data (persistent, lives with the file).
 * Returns: 0 success, -1 src not found / not a file / internal error,
 *         -2 dest already exists, -3 out of memory.
 * -------------------------------------------------------------------------- */
int ramfs_copy_file(const char* src_path, const char* dest_path) {
  if (!src_path || !dest_path) return -1;

  vfs_node_t* src = vfs_lookup(src_path);
  if (!src || src->type != VFS_NODE_FILE) return -1;

  vfs_node_t* existing = vfs_lookup(dest_path);
  if (existing) return -2;

  unsigned int size = src->size;

  char* buf = (char*)malloc(size + 1);
  if (!buf) return -3;

  int nread = vfs_read(src, 0, size, buf);
  if (nread < 0) {
    free(buf);
    return -1;
  }
  buf[nread] = '\0';

  int rc = ramfs_add_ram_file_at_path(dest_path, buf, (unsigned int)nread);
  if (rc != 0) {
    free(buf);
    return -1;
  }
  return 0;
}

/* --------------------------------------------------------------------------
 * Move/rename a node. Instead of copying data, we just relink the parent
 * pointer and update the name — exactly how a real filesystem handles rename
 * within the same partition.
 * Returns: 0 success, -1 error, -2 dest exists.
 * -------------------------------------------------------------------------- */
int ramfs_move_node(const char* src_path, const char* dest_path) {
  if (!src_path || !dest_path) return -1;

  vfs_node_t* src = vfs_lookup(src_path);
  if (!src || !src->fs_private) return -1;

  ramfs_node_t* rn = (ramfs_node_t*)src->fs_private;
  if (rn == &ramfs_root_storage) return -1;

  /* dest_path must be an absolute, fully-resolved path (e.g. /dir/file).
   * The caller (ASM handler) is responsible for the "mv file dir" convention. */
  const char* use_path = dest_path;

  vfs_node_t* existing = vfs_lookup(use_path);
  if (existing) return -2;

  int last_slash = -1;
  for (int i = 0; use_path[i]; i++)
    if (use_path[i] == '/') last_slash = i;
  if (last_slash < 0) return -1;

  char parent_buf[64];
  char base_buf[32];

  if (last_slash == 0) {
    parent_buf[0] = '/';
    parent_buf[1] = '\0';
  } else {
    int i;
    for (i = 0; i < last_slash && i < 63; i++)
      parent_buf[i] = use_path[i];
    parent_buf[i] = '\0';
  }

  int j = 0;
  for (int i = last_slash + 1; use_path[i] && j < 31; i++, j++)
    base_buf[j] = use_path[i];
  base_buf[j] = '\0';
  if (base_buf[0] == '\0') return -1;

  vfs_node_t* dest_parent = vfs_lookup(parent_buf);
  if (!dest_parent || dest_parent->type != VFS_NODE_DIR) return -1;

  ramfs_node_t* dest_parent_rn = (ramfs_node_t*)dest_parent->fs_private;
  if (!dest_parent_rn) return -1;

  /* Relink: update internal ramfs parent and name */
  rn->parent = dest_parent_rn;
  ramfs_strcpy(rn->name, base_buf, 32);

  /* Update the VFS node to match */
  vfs_node_t* vn = ramfs_get_vnode_for(rn);
  if (vn) {
    vn->parent = dest_parent;
    ramfs_strcpy(vn->name, base_buf, 32);
  }

  return 0;
}

/* --------------------------------------------------------------------------
 * Remove a file or empty directory. Marks the node as deleted (name[0] = 0)
 * so readdir/lookup no longer return it.
 * Returns: 0 success, -1 cannot remove root, -2 directory not empty.
 * -------------------------------------------------------------------------- */
int ramfs_remove_node(vfs_node_t* node) {
  if (!node || !node->fs_private) return -1;

  ramfs_node_t* rn = (ramfs_node_t*)node->fs_private;
  if (rn == &ramfs_root_storage) return -1; /* cannot remove root */

  if (rn->type == VFS_NODE_DIR) {
    for (unsigned int i = 0; i < ramfs_file_count; i++) {
      if (ramfs_nodes[i].parent != rn) continue;
      if (ramfs_nodes[i].name[0] != 0) return -2; /* directory not empty */
    }
  }

  rn->name[0] = '\0';
  vfs_node_t* vn = ramfs_get_vnode_for(rn);
  if (vn) vn->name[0] = '\0';
  return 0;
}

/* --------------------------------------------------------------------------
 * Remove a directory and all its contents, or a single file.
 * Returns: 0 success, -1 cannot remove root.
 * -------------------------------------------------------------------------- 
int ramfs_remove_node_recursive(vfs_node_t* node) {
  if (!node || !node->fs_private) return -1;

  ramfs_node_t* rn = (ramfs_node_t*)node->fs_private;
  if (rn == &ramfs_root_storage) return -1;

  if (node->type == VFS_NODE_DIR) {
    vfs_dir_entry_t entry;
    unsigned int index = 0;
    while (vfs_readdir(node, index, &entry) == 0) {
      vfs_node_t* child = node->ops->lookup(node, entry.name);
      if (child)
        ramfs_remove_node_recursive(child);
      index++;
    }
  }
  return ramfs_remove_node(node);
}*/

/* --------------------------------------------------------------------------
 * ramfs_find — Recursive search by exact name
 * -------------------------------------------------------------------------- */

#define FIND_MAX_RESULTS 8
#define FIND_PATH_MAX    48

static char find_results[FIND_MAX_RESULTS][FIND_PATH_MAX];
static int  find_count;

static int is_ancestor(ramfs_node_t* node, ramfs_node_t* ancestor) {
  ramfs_node_t* cur = node;
  while (cur) {
    if (cur == ancestor) return 1;
    cur = cur->parent;
  }
  return 0;
}

static void build_find_path(ramfs_node_t* rn, char* buf, int max) {
  ramfs_node_t* stack[16];
  int depth = 0;

  ramfs_node_t* cur = rn;
  while (cur && cur != &ramfs_root_storage && depth < 16) {
    stack[depth++] = cur;
    cur = cur->parent;
  }

  int pos = 0;
  if (pos < max - 1) buf[pos++] = '/';

  for (int i = depth - 1; i >= 0; i--) {
    const char* n = stack[i]->name;
    int j = 0;
    while (n[j] && pos < max - 1)
      buf[pos++] = n[j++];
    if (i > 0 && pos < max - 1)
      buf[pos++] = '/';
  }
  buf[pos] = 0;
}

int ramfs_find(const char* start_path, const char* name) {
  find_count = 0;
  if (!name || name[0] == 0) return 0;

  ramfs_node_t* start_rn = &ramfs_root_storage;
  if (start_path && start_path[0] != 0 &&
      !(start_path[0] == '/' && start_path[1] == 0)) {
    vfs_node_t* sv = vfs_lookup(start_path);
    if (!sv || !sv->fs_private) return 0;
    start_rn = (ramfs_node_t*)sv->fs_private;
  }

  for (unsigned int i = 0; i < ramfs_file_count && find_count < FIND_MAX_RESULTS; i++) {
    if (ramfs_nodes[i].name[0] == 0) continue;
    if (ramfs_strcmp(ramfs_nodes[i].name, name) != 0) continue;
    if (!is_ancestor(&ramfs_nodes[i], start_rn)) continue;

    build_find_path(&ramfs_nodes[i], find_results[find_count], FIND_PATH_MAX);
    find_count++;
  }
  return find_count;
}

const char* ramfs_find_get_result(int index) {
  if (index < 0 || index >= find_count) return 0;
  return find_results[index];
}