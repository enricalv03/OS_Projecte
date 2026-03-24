/* =============================================================================
 * kernel/ipc/pipe.c — anonymous pipe implementation
 * =============================================================================
 * Uses a fixed-size circular ring buffer allocated from the kernel heap.
 * Both the read and write VFS nodes share a pointer to the same pipe_t.
 *
 * Read semantics:
 *   - Returns available bytes immediately (non-blocking for now).
 *   - Returns 0 (EOF) when the write end is closed and the buffer is empty.
 *
 * Write semantics:
 *   - Writes as many bytes as fit; returns 0 if the buffer is full and the
 *     read end is still open (caller should retry/yield).
 *   - Returns -1 if the read end is closed (broken pipe).
 *
 * This is a simplified implementation: no blocking (no sleep/wake).  A full
 * implementation would call scheduler_block_current() when the buffer is full
 * (write) or empty (read) and wake the peer via scheduler_wake_sleepers().
 * =========================================================================== */

#include "pipe.h"
#include "memory/heap.h"
#include "lib/kstring.h"

/* ---- Forward declaration of VFS ops -------------------------------------- */
static int pipe_read_op (vfs_node_t*, unsigned int, unsigned int, void*);
static int pipe_write_op(vfs_node_t*, unsigned int, unsigned int, const void*);

static vfs_ops_t pipe_read_ops = {
    0, 0, pipe_read_op,  0,              0, 0
};
static vfs_ops_t pipe_write_ops = {
    0, 0, 0,             pipe_write_op,  0, 0
};

/* ---- Allocate a new pipe ------------------------------------------------- */
pipe_t* pipe_create(void) {
    pipe_t* p = (pipe_t*)malloc(sizeof(pipe_t));
    if (!p) return 0;

    memset(p, 0, sizeof(pipe_t));
    p->read_pos   = 0;
    p->write_pos  = 0;
    p->ref_read   = 1;
    p->ref_write  = 1;

    /* Set up the read-end VFS node. */
    vfs_node_t* rn = &p->read_node;
    memset(rn, 0, sizeof(vfs_node_t));
    rn->name[0]   = 'r'; rn->name[1] = 0;
    rn->type      = VFS_NODE_FILE;
    rn->size      = 0;
    rn->fs_private = (void*)p;
    rn->ops       = &pipe_read_ops;
    rn->parent    = 0;

    /* Set up the write-end VFS node. */
    vfs_node_t* wn = &p->write_node;
    memset(wn, 0, sizeof(vfs_node_t));
    wn->name[0]   = 'w'; wn->name[1] = 0;
    wn->type      = VFS_NODE_FILE;
    wn->size      = 0;
    wn->fs_private = (void*)p;
    wn->ops       = &pipe_write_ops;
    wn->parent    = 0;

    return p;
}

/* ---- Read from pipe ------------------------------------------------------- */
static int pipe_read_op(vfs_node_t* node, unsigned int offset,
                        unsigned int size, void* buffer) {
    (void)offset;
    pipe_t* p = (pipe_t*)node->fs_private;
    if (!p || size == 0) return 0;

    unsigned int avail = pipe_count(p);
    if (avail == 0) {
        /* EOF if write end is closed. */
        if (!p->ref_write) return 0;
        /* Non-blocking: no data available yet. */
        return 0;
    }

    if (size > avail) size = avail;

    unsigned char* dst = (unsigned char*)buffer;
    for (unsigned int i = 0; i < size; i++) {
        dst[i] = p->buf[p->read_pos & PIPE_BUF_MASK];
        p->read_pos++;
    }
    return (int)size;
}

/* ---- Write to pipe -------------------------------------------------------- */
static int pipe_write_op(vfs_node_t* node, unsigned int offset,
                         unsigned int size, const void* buffer) {
    (void)offset;
    pipe_t* p = (pipe_t*)node->fs_private;
    if (!p || size == 0) return 0;

    if (!p->ref_read) return -1;  /* broken pipe */

    unsigned int space = pipe_space(p);
    if (space == 0) return 0;     /* buffer full, caller must retry */

    if (size > space) size = space;

    const unsigned char* src = (const unsigned char*)buffer;
    for (unsigned int i = 0; i < size; i++) {
        p->buf[p->write_pos & PIPE_BUF_MASK] = src[i];
        p->write_pos++;
    }
    return (int)size;
}
