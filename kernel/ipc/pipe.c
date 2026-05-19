/* =============================================================================
 * kernel/ipc/pipe.c — anonymous pipe implementation
 * =============================================================================
 * Uses a fixed-size circular ring buffer allocated from the kernel heap.
 * Both the read and write VFS nodes share a pointer to the same pipe_t.
 *
 * This version supports cooperative blocking:
 *   - readers block when empty (unless writer closed -> EOF),
 *   - writers block when full (unless reader closed -> broken pipe),
 *   - read/write paths wake one waiter from the opposite side.
 * =========================================================================== */

#include "pipe.h"
#include "memory/heap.h"
#include "lib/kstring.h"
#include "sched/process.h"
#include "sched/scheduler.h"

/* ---- Forward declaration of VFS ops -------------------------------------- */
static int pipe_close_op(vfs_node_t* node);
static int pipe_read_op (vfs_node_t*, unsigned int, unsigned int, void*);
static int pipe_write_op(vfs_node_t*, unsigned int, unsigned int, const void*);

static vfs_ops_t pipe_read_ops = {
    0, pipe_close_op, pipe_read_op,  0,              0, 0
};
static vfs_ops_t pipe_write_ops = {
    0, pipe_close_op, 0,             pipe_write_op,  0, 0
};

static void pipe_enqueue_waiter(wait_queue_t* q, pcb_t* proc) {
    wait_queue_enqueue_unique(q, proc);
}

static int pipe_wake_one_waiter(wait_queue_t* q) {
    for (;;) {
        pcb_t* p = wait_queue_pop(q);
        if (!p) {
            break;
        }
        if (p->pid != 0 && scheduler_wake_process(p->pid) == 0) {
            return 1;
        }
    }
    return 0;
}

static void pipe_wake_all_waiters(wait_queue_t* q) {
    while (pipe_wake_one_waiter(q)) { }
}

/* ---- Allocate a new pipe ------------------------------------------------- */
pipe_t* pipe_create(void) {
    pipe_t* p = (pipe_t*)malloc(sizeof(pipe_t));
    if (!p) return 0;

    memset(p, 0, sizeof(pipe_t));
    p->read_pos   = 0;
    p->write_pos  = 0;
    p->ref_read   = 1;
    p->ref_write  = 1;
    wait_queue_init(&p->read_waiters);
    wait_queue_init(&p->write_waiters);

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

static int pipe_close_op(vfs_node_t* node) {
    pipe_t* p = (pipe_t*)node->fs_private;
    if (!p) return 0;

    if (node == &p->read_node) {
        if (p->ref_read > 0) {
            p->ref_read--;
        }
        /* Writers blocked on full buffer should wake and see broken pipe. */
        pipe_wake_all_waiters(&p->write_waiters);
    } else if (node == &p->write_node) {
        if (p->ref_write > 0) {
            p->ref_write--;
        }
        /* Readers blocked on empty buffer should wake and see EOF. */
        pipe_wake_all_waiters(&p->read_waiters);
    }

    if (p->ref_read == 0 && p->ref_write == 0) {
        free(p);
    }
    return 0;
}

/* ---- Read from pipe ------------------------------------------------------- */
static int pipe_read_op(vfs_node_t* node, unsigned int offset,
                        unsigned int size, void* buffer) {
    (void)offset;
    pipe_t* p = (pipe_t*)node->fs_private;
    if (!p || size == 0) return 0;
    pcb_t* cur = process_get_current();
    if (!cur || cur->pid == 0) return 0;

    for (;;) {
        unsigned int avail = pipe_count(p);
        if (avail == 0) {
            /* EOF if write end is closed. */
            if (!p->ref_write) return 0;
            pipe_enqueue_waiter(&p->read_waiters, cur);
            scheduler_block_current();
            continue;
        }

        if (size > avail) size = avail;

        unsigned char* dst = (unsigned char*)buffer;
        for (unsigned int i = 0; i < size; i++) {
            dst[i] = p->buf[p->read_pos & PIPE_BUF_MASK];
            p->read_pos++;
        }

        pipe_wake_one_waiter(&p->write_waiters);
        return (int)size;
    }
}

/* ---- Write to pipe -------------------------------------------------------- */
static int pipe_write_op(vfs_node_t* node, unsigned int offset,
                         unsigned int size, const void* buffer) {
    (void)offset;
    pipe_t* p = (pipe_t*)node->fs_private;
    if (!p || size == 0) return 0;
    pcb_t* cur = process_get_current();
    if (!cur || cur->pid == 0) return 0;

    for (;;) {
        if (!p->ref_read) return -1;  /* broken pipe */

        unsigned int space = pipe_space(p);
        if (space == 0) {
            pipe_enqueue_waiter(&p->write_waiters, cur);
            scheduler_block_current();
            continue;
        }

        if (size > space) size = space;

        const unsigned char* src = (const unsigned char*)buffer;
        for (unsigned int i = 0; i < size; i++) {
            p->buf[p->write_pos & PIPE_BUF_MASK] = src[i];
            p->write_pos++;
        }

        pipe_wake_one_waiter(&p->read_waiters);
        return (int)size;
    }
}
