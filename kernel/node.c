/* =============================================================================
 * kernel/node.c — per-core kernel state
 * =============================================================================
 * Stores an array of up to MAX_NODES kernel_node_t objects — one per logical
 * CPU core.  On single-core (and without SMP boot) only node_array[0] is used.
 *
 * On x86 SMP: node_get_current_node() reads the LAPIC ID (via the
 * lapic_get_id() function in smp.c) and uses that as an index.  The mapping
 * from LAPIC ID → node index is stored in lapic_to_node[].
 *
 * On ARM SMP (future): node_get_current_node() would read MPIDR_EL1.
 * =========================================================================== */

#include "node.h"

/* Global node array — index 0 = BSP, 1..N = APs. */
static kernel_node_t node_array[MAX_NODES];

/* How many nodes have been initialised (1 = BSP only). */
static unsigned int node_count = 0;

/* LAPIC-ID → node-array-index mapping table (MAX LAPIC ID = 255). */
#define MAX_LAPIC_ID 256
static unsigned int lapic_to_node[MAX_LAPIC_ID];

/* node_init() — called once by the BSP during early kernel init.
 * Must be called before any other node_* function. */
void node_init(void) {
    for (unsigned int i = 0; i < MAX_LAPIC_ID; i++) lapic_to_node[i] = 0;
    for (unsigned int i = 0; i < (unsigned int)MAX_NODES; i++) {
        node_array[i].node_id   = i;
        node_array[i].current   = 0;
        node_array[i].ticks     = 0;
        node_array[i].mbox_head = 0;
        node_array[i].mbox_tail = 0;
    }
    lapic_to_node[0] = 0;   /* BSP: LAPIC ID 0 → node 0 */
    node_count = 1;
}

/* Return the node for the currently-executing CPU.
 * On single-core this is always node_array[0].
 * On SMP it reads the LAPIC ID and looks it up in lapic_to_node[]. */
kernel_node_t* node_get_current_node(void) {
#ifdef CONFIG_SMP
    /* Forward-declared in smp.h; avoid the include cycle by using a weak decl. */
    extern unsigned int lapic_get_id(void);
    unsigned int id = lapic_get_id();
    if (id < MAX_LAPIC_ID) {
        return &node_array[lapic_to_node[id]];
    }
#endif
    return &node_array[0];
}

/* Called by ap_kernel_main() in smp.c after an AP boots. */
kernel_node_t* node_init_ap(unsigned int lapic_id) {
    if (node_count >= (unsigned int)MAX_NODES) return 0;
    if (lapic_id >= MAX_LAPIC_ID) return 0;

    unsigned int idx = node_count;
    node_array[idx].node_id   = idx;
    node_array[idx].current   = 0;
    node_array[idx].ticks     = 0;
    node_array[idx].mbox_head = 0;
    node_array[idx].mbox_tail = 0;

    lapic_to_node[lapic_id] = idx;
    node_count++;
    return &node_array[idx];
}

/* ---- Lock-free SPSC mailbox ------------------------------------------------
 *
 * One producer (the remote CPU calling node_send_message) and one consumer
 * (the owning CPU calling node_recv_message).  No spinlock needed because:
 *   - The producer reads mbox_head (load-acquire) to check for space, then
 *     fills the slot and does a store-release on mbox_tail.
 *   - The consumer reads mbox_tail (load-acquire) to check for data, then
 *     copies the slot and does a store-release on mbox_head.
 *
 * We use GCC __sync_synchronize() (full memory barrier) to guarantee
 * the required ordering on both x86 and ARM without pulling in stdatomic.h.
 * On x86 the barrier usually compiles away because x86 has TSO ordering;
 * on ARM it maps to DMB instructions.
 * -------------------------------------------------------------------------- */

int node_send_message(unsigned int dest_node_id, unsigned int type,
                      const unsigned int data[4]) {
    if (dest_node_id >= node_count) return -1;

    kernel_node_t* node = &node_array[dest_node_id];

    /* Load-acquire head to check available space. */
    unsigned int head = node->mbox_head;
    __sync_synchronize();   /* acquire barrier */
    unsigned int tail = node->mbox_tail;

    if ((tail - head) >= NODE_MBOX_SIZE) return -1;  /* ring full */

    /* Fill the slot — visible to the consumer only after the barrier below. */
    unsigned int slot = tail & NODE_MBOX_MASK;
    node->mbox[slot].sender_node_id = node_get_id();
    node->mbox[slot].type           = type;
    if (data) {
        node->mbox[slot].data[0] = data[0];
        node->mbox[slot].data[1] = data[1];
        node->mbox[slot].data[2] = data[2];
        node->mbox[slot].data[3] = data[3];
    }

    /* Store-release: make the filled slot visible before advancing tail. */
    __sync_synchronize();
    node->mbox_tail = tail + 1;
    return 0;
}

int node_recv_message(node_message_t* out) {
    kernel_node_t* node = node_get_current_node();

    /* Load-acquire tail to check if data is available. */
    unsigned int tail = node->mbox_tail;
    __sync_synchronize();   /* acquire barrier */
    unsigned int head = node->mbox_head;

    if (head == tail) return -1;  /* ring empty */

    /* Copy the slot before advancing head (so the producer sees space). */
    unsigned int slot = head & NODE_MBOX_MASK;
    *out = node->mbox[slot];

    /* Store-release: make the consumed slot available to the producer. */
    __sync_synchronize();
    node->mbox_head = head + 1;
    return 0;
}
