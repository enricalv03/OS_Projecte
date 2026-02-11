#include "vfs.h"

/* --------------------------------------------------------------------------
 * Internal state
 * -------------------------------------------------------------------------- */

static vfs_node_t* vfs_root = 0;

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

static int vfs_strncmp(const char* a, const char* b, int n) {
  if (!a || !b) return (a == b) ? 0 : 1;
  for (int i = 0; i < n; i++) {
    char ca = a[i];
    char cb = b[i];
    if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
    if (ca == 0 || cb == 0) return 0;
  }
  return 0;
}

/* --------------------------------------------------------------------------
 * VFS core
 * -------------------------------------------------------------------------- */

int vfs_mount_root(vfs_node_t* root) {
  if (!root || root->type != VFS_NODE_DIR) {
    return -1;
  }
  vfs_root = root;
  return 0;
}

vfs_node_t* vfs_get_root(void) {
  return vfs_root;
}

int vfs_read(vfs_node_t* node,
             unsigned int offset,
             unsigned int size,
             void* buffer) {
  if (!node || !node->ops || !node->ops->read) {
    return -1;
  }
  return node->ops->read(node, offset, size, buffer);
}

int vfs_readdir(vfs_node_t* dir,
                unsigned int index,
                vfs_dir_entry_t* out) {
  if (!dir || dir->type != VFS_NODE_DIR || !dir->ops || !dir->ops->readdir) {
    return -1;
  }
  return dir->ops->readdir(dir, index, out);
}

/* Very simple path lookup:
 *  - Only supports absolute paths like "/name"
 *  - Single level (no subdirectories yet)
 */
vfs_node_t* vfs_lookup(const char* path) {
  if (!vfs_root || !path) {
    return 0;
  }

  /* Handle "/" and empty path: return root */
  if (path[0] == 0 || (path[0] == '/' && path[1] == 0)) {
    return vfs_root;
  }

  /* Skip leading '/' if present */
  if (path[0] == '/') {
    path++;
  }

  /* Extract first component into a small buffer */
  char name[32];
  int i = 0;
  while (path[0] != 0 && path[0] != '/' && i < (int)sizeof(name) - 1) {
    name[i++] = *path++;
  }
  name[i] = 0;

  /* We only support single-level paths for now:
   * if there is another '/', we fail.
   */
  if (*path == '/') {
    return 0;
  }

  /* Iterate directory entries using readdir */
  if (!vfs_root->ops || !vfs_root->ops->readdir) {
    return 0;
  }

  /* Use the filesystem's lookup callback if available */
  if (vfs_root->ops && vfs_root->ops->lookup) {
    return vfs_root->ops->lookup(vfs_root, name);
  }

  return 0;
}


