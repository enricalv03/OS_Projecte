#ifndef PIPE_H
#define PIPE_H

/* =============================================================================
 * kernel/ipc/pipe.h — anonymous pipe
 * =============================================================================
 * An anonymous pipe is a one-way kernel ring buffer connecting a writer
 * process to a reader process.  Both ends are exposed as VFS nodes (type
 * VFS_NODE_FILE) backed by pipe_ops.  A process uses sys_pipe() to allocate
 * a pipe, receiving two file descriptors: pipefd[0] = read end, pipefd[1] =
 * write end.
 * =========================================================================== */

#include "fs/vfs.h"

#define PIPE_BUF_SIZE   4096   /* ring buffer capacity in bytes (power of 2)  */
#define PIPE_BUF_MASK   (PIPE_BUF_SIZE - 1)

typedef struct {
    unsigned char buf[PIPE_BUF_SIZE];
    volatile unsigned int read_pos;   /* consumer index  */
    volatile unsigned int write_pos;  /* producer index  */
    unsigned int ref_read;            /* 1 if read-end is open  */
    unsigned int ref_write;           /* 1 if write-end is open */

    vfs_node_t read_node;
    vfs_node_t write_node;
} pipe_t;

/* Allocate and initialise a new pipe.
 * Returns a pointer to the new pipe, or NULL if memory is unavailable. */
pipe_t* pipe_create(void);

/* Return the number of bytes currently in the ring buffer. */
static inline unsigned int pipe_count(const pipe_t* p) {
    return p->write_pos - p->read_pos;
}

/* Return the remaining space in the ring buffer. */
static inline unsigned int pipe_space(const pipe_t* p) {
    return PIPE_BUF_SIZE - (p->write_pos - p->read_pos);
}

#endif
