/* =============================================================================
 * kernel/fs/devfs.c — pseudo-device filesystem
 * =============================================================================
 * Device nodes are backed by custom vfs_ops_t tables; no on-disk data is
 * needed.  Each device is represented by a vfs_node_t with type=VFS_NODE_FILE
 * and a unique ops table that implements the device semantics.
 * =========================================================================== */

#include "devfs.h"
#include "vfs.h"
#include "ramfs.h"
#include "memory/heap.h"

/* ---- External console helpers (defined in kernel.asm) ---- */
extern void console_putchar_syscall(char c, unsigned char attr);
extern void console_newline_syscall(void);
extern int  kbd_buffer_get(void);

/* ---- /dev/null ------------------------------------------------------------ */

static int dev_null_read(vfs_node_t* node, unsigned int offset,
                         unsigned int size, void* buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0;  /* EOF immediately */
}

static int dev_null_write(vfs_node_t* node, unsigned int offset,
                          unsigned int size, const void* buffer) {
    (void)node; (void)offset; (void)buffer;
    return (int)size;  /* accept and discard everything */
}

static vfs_ops_t dev_null_ops = {
    0,              /* open    */
    0,              /* close   */
    dev_null_read,
    dev_null_write,
    0,              /* readdir */
    0               /* lookup  */
};

/* ---- /dev/zero ------------------------------------------------------------ */

static int dev_zero_read(vfs_node_t* node, unsigned int offset,
                         unsigned int size, void* buffer) {
    (void)node; (void)offset;
    char* dst = (char*)buffer;
    for (unsigned int i = 0; i < size; i++) dst[i] = 0;
    return (int)size;
}

static vfs_ops_t dev_zero_ops = {
    0,              /* open    */
    0,              /* close   */
    dev_zero_read,
    dev_null_write, /* writes discarded (same as /dev/null) */
    0,              /* readdir */
    0               /* lookup  */
};

/* ---- /dev/console --------------------------------------------------------- */

static int dev_console_read(vfs_node_t* node, unsigned int offset,
                            unsigned int size, void* buffer) {
    (void)node; (void)offset;
    char* dst = (char*)buffer;
    unsigned int n = 0;
    for (unsigned int i = 0; i < size; i++) {
        int ch = kbd_buffer_get();
        if (ch < 0) break;
        dst[n++] = (char)ch;
    }
    return (int)n;
}

static int dev_console_write(vfs_node_t* node, unsigned int offset,
                             unsigned int size, const void* buffer) {
    (void)node; (void)offset;
    const char* src = (const char*)buffer;
    for (unsigned int i = 0; i < size; i++) {
        char c = src[i];
        if (c == '\n')
            console_newline_syscall();
        else if (c != '\0')
            console_putchar_syscall(c, 0x0F);
    }
    return (int)size;
}

static vfs_ops_t dev_console_ops = {
    0,                  /* open    */
    0,                  /* close   */
    dev_console_read,
    dev_console_write,
    0,                  /* readdir */
    0                   /* lookup  */
};

/* ---- /dev/random ---------------------------------------------------------- */

/* Linear-congruential PRNG — good enough for a demo device.
 * Not cryptographically secure; use a hardware TRNG for production. */
static unsigned int prng_state = 0xDEADBEEFu;

static unsigned int prng_next(void) {
    prng_state = prng_state * 1664525u + 1013904223u;
    return prng_state;
}

static int dev_random_read(vfs_node_t* node, unsigned int offset,
                           unsigned int size, void* buffer) {
    (void)node; (void)offset;
    unsigned char* dst = (unsigned char*)buffer;
    for (unsigned int i = 0; i < size; i++) {
        if ((i & 3) == 0) prng_next();  /* advance PRNG every 4 bytes */
        dst[i] = (unsigned char)(prng_state >> ((i & 3) * 8));
    }
    return (int)size;
}

static vfs_ops_t dev_random_ops = {
    0,              /* open    */
    0,              /* close   */
    dev_random_read,
    dev_null_write, /* writes ignored */
    0,              /* readdir */
    0               /* lookup  */
};

/* ---- Static VFS nodes for each device ------------------------------------ */

/* We need one vfs_node_t per device.  They live in BSS (static lifetime). */
static vfs_node_t dev_null_node;
static vfs_node_t dev_zero_node;
static vfs_node_t dev_console_node;
static vfs_node_t dev_random_node;

/* ---- Small string copy helper -------------------------------------------- */
static void devfs_strcpy(char* dst, const char* src, unsigned int max) {
    unsigned int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* ---- devfs_init ----------------------------------------------------------- */

static void devfs_node_init(vfs_node_t* n, const char* name, vfs_ops_t* ops,
                            vfs_node_t* parent) {
    devfs_strcpy(n->name, name, sizeof(n->name));
    n->type       = VFS_NODE_FILE;
    n->size       = 0;
    n->fs_private = 0;  /* no backing store */
    n->ops        = ops;
    n->parent     = parent;
}

void devfs_init(void) {
    /* Create /dev directory in RAMFS. */
    ramfs_mkdir_at_path("/dev");

    /* Locate the /dev directory node so we can set parent pointers. */
    vfs_node_t* dev_dir = vfs_lookup("/dev");

    /* Initialise device nodes. */
    devfs_node_init(&dev_null_node,    "null",    &dev_null_ops,    dev_dir);
    devfs_node_init(&dev_zero_node,    "zero",    &dev_zero_ops,    dev_dir);
    devfs_node_init(&dev_console_node, "console", &dev_console_ops, dev_dir);
    devfs_node_init(&dev_random_node,  "random",  &dev_random_ops,  dev_dir);

    /* Register with RAMFS so vfs_lookup("/dev/null") etc. work.
     * We use the "RAM-backed, size=0" entry as a placeholder and then
     * immediately replace its ops pointer with the device ops.
     * ramfs_add_ram_file_under installs a new vfs_node_t via the RAMFS
     * vnode table; that vnode is what vfs_lookup returns.  We then patch
     * its ops to point to the real device ops. */

    if (dev_dir) {
        /* Add null device */
        ramfs_add_ram_file_under(dev_dir, "null",    0, 0);
        vfs_node_t* n = vfs_lookup("/dev/null");
        if (n) { n->ops = &dev_null_ops; }

        /* Add zero device */
        ramfs_add_ram_file_under(dev_dir, "zero",    0, 0);
        vfs_node_t* z = vfs_lookup("/dev/zero");
        if (z) { z->ops = &dev_zero_ops; }

        /* Add console device */
        ramfs_add_ram_file_under(dev_dir, "console", 0, 0);
        vfs_node_t* c = vfs_lookup("/dev/console");
        if (c) { c->ops = &dev_console_ops; }

        /* Add random device */
        ramfs_add_ram_file_under(dev_dir, "random",  0, 0);
        vfs_node_t* r = vfs_lookup("/dev/random");
        if (r) { r->ops = &dev_random_ops; }
    }
}
