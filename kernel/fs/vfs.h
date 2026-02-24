#ifndef VFS_H
#define VFS_H

/* Simple Virtual File System interface */

typedef enum {
  VFS_NODE_FILE = 1,
  VFS_NODE_DIR  = 2
} vfs_node_type_t;

struct vfs_node;

typedef struct vfs_dir_entry {
  char name[32];
  vfs_node_type_t type;
} vfs_dir_entry_t;

typedef struct vfs_ops {
  int (*open)(struct vfs_node* node);
  int (*close)(struct vfs_node* node);
  int (*read)(struct vfs_node* node,
              unsigned int offset,
              unsigned int size,
              void* buffer);
  int (*write)(struct vfs_node* node,
               unsigned int offset,
               unsigned int size,
               const void* buffer);
  int (*readdir)(struct vfs_node* node,
                 unsigned int index,
                 vfs_dir_entry_t* out);
  /* Lookup a child node by name within a directory */
  struct vfs_node* (*lookup)(struct vfs_node* dir, const char* name);
} vfs_ops_t;

typedef struct vfs_node {
  char name[32];
  vfs_node_type_t type;
  unsigned int size;
  void* fs_private;
  vfs_ops_t* ops;
  struct vfs_node* parent;
} vfs_node_t;

/* VFS core API */

int vfs_mount_root(vfs_node_t* root);
vfs_node_t* vfs_get_root(void);
vfs_node_t* vfs_get_cwd(void);
void vfs_set_cwd(vfs_node_t* node);
vfs_node_t* vfs_lookup(const char* path);

int vfs_read(vfs_node_t* node,
             unsigned int offset,
             unsigned int size,
             void* buffer);

int vfs_readdir(vfs_node_t* dir,
                unsigned int index,
                vfs_dir_entry_t* out);

/* Return directory entry type: 1 = file, 2 = directory. Use from ASM to avoid struct layout. */
int vfs_dir_entry_get_type(const vfs_dir_entry_t* entry);
void vfs_set_cwd_with_path(vfs_node_t* node, const char* path);
const char* vfs_get_cwd_path(void);
void vfs_chdir_parent(void);

#endif


