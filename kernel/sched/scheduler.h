#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

// MLFQ Configuration
#define MLFQ_QUEUES           5
#define MLFQ_TIMESLICE_BASE   10
#define MLFQ_BOOST_INTERVAL   1000

// CFS Configuration
#define CFS_MIN_GRANULARITY   1
#define CFS_TARGET_LATENCY    6
#define CFS_WEIGHT_DEFAULT    1024

// Scheduler functions
void scheduler_init(void);
void scheduler_tick(void);
pcb_t* scheduler_next(void);
void scheduler_enqueue(pcb_t* process);
void scheduler_dequeue(pcb_t* process);
void scheduler_yield(void);            
void scheduler_boost(void);

// Blocked process management
void scheduler_block_current(void);
void scheduler_wake_sleepers(unsigned int current_ticks);
void scheduler_wake_parent(unsigned int child_pid);

// Statistics
unsigned int scheduler_get_load(void);

// Set current running process (for kernel initialization)
void scheduler_set_current(pcb_t* process);

/* SMP: migrate process to another node's run queue and send it an IPI.
 * Returns 0 on success, -1 if destination node not found or mailbox full. */
int scheduler_migrate(unsigned int pid, unsigned int dest_node_id);

/* SMP: called by each AP on receipt of a NODE_MSG_MIGRATE message to
 * pick up the migrated process and enqueue it locally. */
void scheduler_handle_migrate(unsigned int pid);

/* SMP: process incoming inter-node messages for this CPU's node. */
void scheduler_process_messages(void);

#endif