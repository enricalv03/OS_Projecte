#include "vfs.h"
#include "ramfs.h"

/* --------------------------------------------------------------------------
 * Internal state
 * -------------------------------------------------------------------------- */

static vfs_node_t* vfs_root = 0;
static vfs_node_t* vfs_cwd = 0;

#define VFS_CWD_PATH_SIZE 64
#define VFS_CWD_CHAIN_MAX 32
static char vfs_cwd_path[VFS_CWD_PATH_SIZE] = "/";

/* --------------------------------------------------------------------------
 * Small local string helpers (no libc)
 * -------------------------------------------------------------------------- */

static int vfs_strlen(const char* s) {
  int len = 0;
  if (!s) return 0;
  while (s[len] != 0) {
    len++;
  }
  return len;
}

/* Path buffer for prepending '/' when path has no leading slash */
#define VFS_PATH_BUF_SIZE 64
static char vfs_path_buf[VFS_PATH_BUF_SIZE];

/* Fill vfs_cwd_path from the vnode parent chain so it always matches vfs_cwd.
 * If only vfs_set_cwd(dir) was used (no explicit path), the old code left
 * vfs_cwd_path stale (often "/"). Then ls used the vnode (empty home) while
 * write/cat used the string path (root) — empty listings and "missing" files. */
static void vfs_sync_cwd_path_from_node(vfs_node_t* node) {
  vfs_node_t* root = vfs_get_root();
  if (!node || node == root) {
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
    return;
  }

  vfs_node_t* chain[VFS_CWD_CHAIN_MAX];
  int depth = 0;
  vfs_node_t* p = node;
  while (p && p != root && depth < VFS_CWD_CHAIN_MAX) {
    chain[depth++] = p;
    p = p->parent;
  }
  if (p != root) {
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
    return;
  }

  int pos = 0;
  vfs_cwd_path[pos++] = '/';
  for (int i = depth - 1; i >= 0; i--) {
    const char* nm = chain[i]->name;
    if (!nm)
      nm = "";
    for (int j = 0; nm[j] != '\0' && pos < VFS_CWD_PATH_SIZE - 1; j++)
      vfs_cwd_path[pos++] = nm[j];
    if (i > 0 && pos < VFS_CWD_PATH_SIZE - 1)
      vfs_cwd_path[pos++] = '/';
  }
  vfs_cwd_path[pos] = '\0';
}

/* --------------------------------------------------------------------------
 * VFS core
 * -------------------------------------------------------------------------- */

int vfs_mount_root(vfs_node_t* root) {
  if (!root || root->type != VFS_NODE_DIR) {
    return -1;
  }
  vfs_root = root;
  vfs_set_cwd(root);  /* so vfs_cwd and vfs_cwd_path are set to "/" */
  return 0;
}

vfs_node_t* vfs_get_root(void) {
  if (vfs_root)
    return vfs_root;
  return ramfs_get_vfs_root();
}

vfs_node_t* vfs_get_cwd(void) {
  if (vfs_cwd)
    return vfs_cwd;
  if (vfs_root)
    return vfs_root;
  return ramfs_get_vfs_root();
}

void vfs_set_cwd(vfs_node_t* node) {
  if (!node || node->type != 2)  /* VFS_NODE_DIR */
    return;
  vfs_cwd = node;
  vfs_sync_cwd_path_from_node(node);
}

void vfs_set_cwd_with_path(vfs_node_t* node, const char* path) {
  if (!node || node->type != 2) {
    return;
  }

  /* Keep CWD state canonical: derive text path from the vnode chain instead of
   * trusting a caller-provided string that may be relative, stale, or malformed.
   * This prevents "prompt/path points to / while ls uses another directory". */
  vfs_cwd = node;
  (void)path;
  vfs_sync_cwd_path_from_node(node);
}

const char* vfs_get_cwd_path(void) {
  /* Keep textual CWD in lockstep with the vnode CWD.
   * Some callers depend on the string (path building), others on vfs_cwd
   * (directory listing). If anything changed vfs_cwd without updating the
   * string, refresh lazily here so both views stay consistent. */
  vfs_sync_cwd_path_from_node(vfs_get_cwd());
  return vfs_cwd_path;
}

/* Change CWD to parent directory. At root, stays at "/". */
void vfs_chdir_parent(void) {
  vfs_node_t* root = vfs_get_root();
  if (!root) return;

  vfs_node_t* cwd = vfs_cwd ? vfs_cwd : root;
  if (cwd == root) {
    vfs_cwd = root;
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
    return;
  }

  if (!cwd->parent) {
    vfs_cwd = root;
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
    return;
  }

  vfs_cwd = cwd->parent;

  /* Trim last component from vfs_cwd_path. Cap len to avoid reading past buffer. */
  int len = 0;
  while (len < VFS_CWD_PATH_SIZE && vfs_cwd_path[len] != '\0')
    len++;
  if (len <= 1) {
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
    return;
  }
  int i = len - 1;
  if (i >= VFS_CWD_PATH_SIZE)
    i = VFS_CWD_PATH_SIZE - 1;
  while (i > 0 && vfs_cwd_path[i] == '/')
    i--;
  while (i > 0 && vfs_cwd_path[i] != '/')
    i--;
  if (i <= 0) {
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
  } else {
    vfs_cwd_path[i] = '\0';
  }
}

int vfs_read(vfs_node_t* node,
             unsigned int offset,
             unsigned int size,
             void* buffer) {
  if (!node) return -1;
  if (node->ops && node->ops->read) {
    return node->ops->read(node, offset, size, buffer);
  }
  /* Fallback for RAMFS nodes when vnode ops pointers are missing. */
  return ramfs_read_fallback(node, offset, size, buffer);
}

int vfs_write(vfs_node_t* node,
              unsigned int offset,
              unsigned int size,
              const void* buffer) {
  if (!node || !node->ops || !node->ops->write)
    return -1;
  return node->ops->write(node, offset, size, buffer);
}

vfs_node_t* vfs_create(const char* path) {
  if (!path) return 0;

  /* Check if it already exists. */
  vfs_node_t* existing = vfs_lookup(path);
  if (existing) return existing;

  /* Find the parent directory and basename. */
  /* Locate last '/' to split into dir + name. */
  int len = vfs_strlen(path);
  int sep = -1;
  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') { sep = i; break; }
  }

  const char* name;
  char parent_path[64];
  if (sep <= 0) {
    /* File in root. */
    parent_path[0] = '/'; parent_path[1] = 0;
    name = (sep == 0) ? path + 1 : path;
  } else {
    int plen = sep;
    if (plen >= 64) plen = 63;
    for (int i = 0; i < plen; i++) parent_path[i] = path[i];
    parent_path[plen] = 0;
    name = path + sep + 1;
  }

  if (name[0] == 0) return 0;

  /* ramfs_add_ram_file_at_path accepts the full path; use it to create
   * an empty file. */
  if (ramfs_add_ram_file_at_path(path, 0, 0) != 0) {
    /* May have failed because parent dir doesn't exist — try cwd-relative. */
    return 0;
  }
  (void)parent_path;
  return vfs_lookup(path);
}

int vfs_truncate(vfs_node_t* node, unsigned int new_size) {
  /* The only filesystem that supports truncate today is ramfs. */
  extern int ramfs_truncate(vfs_node_t*, unsigned int);
  return ramfs_truncate(node, new_size);
}

int vfs_readdir(vfs_node_t* dir,
                unsigned int index,
                vfs_dir_entry_t* out) {
  /* Be tolerant about dir->type here so that minor struct-layout or
   * initialization issues don't completely break directory listings.
   * As long as a node provides a readdir implementation, we ask it. */
  if (!dir) return -1;
  if (dir->ops && dir->ops->readdir) {
    return dir->ops->readdir(dir, index, out);
  }
  /* Fallback: resolve listing directly from RAMFS tables. */
  return ramfs_readdir_fallback(dir, index, out);
}

int vfs_dir_entry_get_type(const vfs_dir_entry_t* entry) {
  if (!entry) return 0;
  return (int)entry->type;
}

/* Multi-component path lookup: "/sistem/config", "/home/normal", "/readme.txt"
 * If path does not start with '/', we prepend '/' (so "readme.txt" -> "/readme.txt").
 */
vfs_node_t* vfs_lookup(const char* path) {
  vfs_node_t* root = vfs_get_root();
  if (!root || !path) {
    return 0;
  }

  /* Use path_buf if path doesn't start with '/' */
  if (path[0] != 0 && path[0] != '/') {
    int len = vfs_strlen(path);
    if (len + 2 > VFS_PATH_BUF_SIZE)
      return 0;
    vfs_path_buf[0] = '/';
    for (int i = 0; i <= len; i++)
      vfs_path_buf[i + 1] = path[i];
    path = vfs_path_buf;
  }

  /* "/" or "" -> return root */
  if (path[0] == 0 || (path[0] == '/' && path[1] == 0)) {
    return root;
  }

  /* Skip leading '/' */
  if (path[0] == '/')
    path++;

  vfs_node_t* current = root;

  while (path[0] != 0) {
    /* Extract next component (up to '/' or end) */
    char name[32];
    int i = 0;
    while (path[0] != 0 && path[0] != '/' && i < (int)sizeof(name) - 1) {
      name[i++] = *path++;
    }
    name[i] = 0;

    /* Skip trailing slashes */
    while (path[0] == '/')
      path++;

    /* Empty component (e.g. "//") -> skip */
    if (name[0] == 0)
      continue;

    if (current->ops && current->ops->lookup) {
      current = current->ops->lookup(current, name);
    } else {
      /* Fallback: resolve child from RAMFS metadata if vnode ops are missing. */
      vfs_node_t* next = 0;
      if (ramfs_lookup_child(current, name, &next) != 0) {
        return 0;
      }
      current = next;
    }
    if (!current)
      return 0;
  }

  return current;
}
