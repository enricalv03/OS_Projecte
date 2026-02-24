#include "vfs.h"
#include "ramfs.h"

/* --------------------------------------------------------------------------
 * Internal state
 * -------------------------------------------------------------------------- */

static vfs_node_t* vfs_root = 0;
static vfs_node_t* vfs_cwd = 0;

#define VFS_CWD_PATH_SIZE 64
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
  if (node == vfs_get_root())
  {
    vfs_cwd_path[0] = '/';
    vfs_cwd_path[1] = '\0';
  }
  
}

void vfs_set_cwd_with_path(vfs_node_t* node, const char* path) {
  if (!node || node->type != 2 || !path)
  {
    return;
  }
  vfs_cwd = node;
  int i = 0;
  while (path[i] != '\0' && i < VFS_CWD_PATH_SIZE - 1)
  {
    vfs_cwd_path[i] = path[i];
    i++;
  }
  if (i >= VFS_CWD_PATH_SIZE)
    i = VFS_CWD_PATH_SIZE - 1;
  vfs_cwd_path[i] = '\0';
}

const char* vfs_get_cwd_path(void) {
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

    if (current->type != VFS_NODE_DIR || !current->ops || !current->ops->lookup) {
      return 0;
    }
    current = current->ops->lookup(current, name);
    if (!current)
      return 0;
  }

  return current;
}
