#ifndef KERNEL_NODE_H
#define KERNEL_NODE_H

/* =============================================================================
 * kernel_node — per-core kernel state abstraction
 * =============================================================================
 * In a multikernel each CPU core runs its own kernel instance ("node").
 * All scheduling decisions, process lookups, and future inter-core messaging
 * go through this struct rather than accessing process/scheduler globals
 * directly.  That way, when we add a second core we just allocate a second
 * kernel_node_t and the rest of the code changes minimally.
 *
 * Current state:  a single node (node_id = 0) backed by the existing
 * scheduler/process infrastructure.
 * =========================================================================== */

#include "sched/process.h"

/* Lock-free SPSC ring capacity.  MUST be a power of two. */
#define NODE_MBOX_SIZE  16
#define NODE_MBOX_MASK  (NODE_MBOX_SIZE - 1)

/* One message slot in the inter-node mailbox. */
typedef struct {
    unsigned int sender_node_id;
    unsigned int type;
    unsigned int data[4];
} node_message_t;

/* Per-core kernel state. */
typedef struct {
    unsigned int    node_id;   /* logical core identifier (0-based)         */
    pcb_t*          current;   /* process currently running on this node     */
    unsigned int    ticks;     /* scheduler ticks on this node               */

    /* Lock-free SPSC ring mailbox.
     * Producer (sender) owns mbox_tail — read freely, written with a store-
     * release after filling the slot.
     * Consumer (this node's CPU) owns mbox_head — read freely, advanced with
     * a store-release after consuming a slot.
     * Invariant: (mbox_tail - mbox_head) <= NODE_MBOX_SIZE at all times.
     * Both indices are monotonically increasing; wrap is done via masking. */
    node_message_t  mbox[NODE_MBOX_SIZE];
    volatile unsigned int mbox_head;   /* consumer index (owned by this CPU) */
    volatile unsigned int mbox_tail;   /* producer index (written by sender) */
} kernel_node_t;

/* --------------------------------------------------------------------------
 * Accessors — always operate on the calling core's node.
 * On SMP, node_get_current_node() will read LAPIC ID / MPIDR to select the
 * right kernel_node_t.  For single-core it always returns &node0.
 * -------------------------------------------------------------------------- */

/* Initialise the node subsystem (call once from BSP before any other node_*). */
void node_init(void);

kernel_node_t* node_get_current_node(void);

static inline pcb_t* node_get_current(void) {
    return node_get_current_node()->current;
}

static inline void node_set_current(pcb_t* p) {
    node_get_current_node()->current = p;
}

static inline unsigned int node_get_id(void) {
    return node_get_current_node()->node_id;
}

static inline unsigned int node_get_ticks(void) {
    return node_get_current_node()->ticks;
}

static inline void node_tick(void) {
    node_get_current_node()->ticks++;
}

/* --------------------------------------------------------------------------
 * Message passing (stub — fully wired when multi-core lands)
 * -------------------------------------------------------------------------- */

/* Inter-node message types */
#define NODE_MSG_NONE     0
#define NODE_MSG_MIGRATE  1    /* data[0]=pid to migrate to this node */
#define NODE_MSG_WAKE     2    /* data[0]=pid to wake (was blocked, now ready) */
#define NODE_MSG_KILL     3    /* data[0]=pid to kill */

/* Send a message to dest_node's mailbox.
 * Returns 0 on success, -1 if mailbox is full or node not found. */
int node_send_message(unsigned int dest_node_id, unsigned int type,
                      const unsigned int data[4]);

/* Receive the next message from this node's mailbox.
 * Returns 0 and fills *out on success, -1 if mailbox is empty. */
int node_recv_message(node_message_t* out);

/* SMP: initialise a new node for an Application Processor.
 * lapic_id — the LAPIC (or MPIDR) identifier of the AP.
 * Returns a pointer to the newly-initialised kernel_node_t, or NULL if all
 * node slots are occupied. */
kernel_node_t* node_init_ap(unsigned int lapic_id);

/* Maximum number of nodes (BSP + APs).  Compile-time limit. */
#ifndef MAX_NODES
#define MAX_NODES 8
#endif

#endif
