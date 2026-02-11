#include "scheduler.h"
#include "process.h"
#include "../lib/kstring.h"

static pcb_t* mlfq_queues[MLFQ_QUEUES];
static unsigned int queue_sizes[MLFQ_QUEUES];

static pcb_t* cfs_list = 0;
static pcb_t* blocked_list = 0;   /* Singly-linked list of blocked processes */

static unsigned int scheduler_ticks = 0;
static unsigned int last_boost = 0;
static pcb_t* current_running = 0;

static unsigned int calculate_timeslice(unsigned int queue_level) {
  return MLFQ_TIMESLICE_BASE * (1 << queue_level);
}

static unsigned int calculate_weight(unsigned int priority) {
  return 256 + (priority * 384);
}

static unsigned int calculate_vruntime_delta(unsigned int runtime, unsigned int weight) {
  return (runtime * CFS_WEIGHT_DEFAULT) / weight;
}

void scheduler_init(void) {
  memset(mlfq_queues, 0, sizeof(mlfq_queues));
  memset(queue_sizes, 0, sizeof(queue_sizes));
  cfs_list = 0;
  blocked_list = 0;
  scheduler_ticks = 0;
  last_boost = 0;
  current_running = 0;
}

static void mlfq_enqueue(pcb_t* process) {
  unsigned int level = process->queue_level;
  if (level >= MLFQ_QUEUES)
  {
    level = MLFQ_QUEUES - 1;
  }

  process->next = mlfq_queues[level];
  if (mlfq_queues[level])
  {
    mlfq_queues[level]->prev = process;
  }
  mlfq_queues[level] = process;
  process->prev = 0;
  queue_sizes[level]++;
}

static void mlfq_dequeue(pcb_t* process) {
  unsigned int level = process->queue_level;
  if (level >= MLFQ_QUEUES) {
      return;
  }
  
  if (process->prev) {
      process->prev->next = process->next;
  } else {
      mlfq_queues[level] = process->next;
  }
  if (process->next) {
      process->next->prev = process->prev;
  }
  process->next = 0;
  process->prev = 0;
  queue_sizes[level]--;
}

static pcb_t* mlfq_next(void) {
  for (int i = MLFQ_QUEUES - 1; i >= 0; i--)
  {
    if (mlfq_queues[i])
    {
      pcb_t* process = mlfq_queues[i];
      mlfq_dequeue(process);
      return process;
    } 
  }
  return 0;
}

static void cfs_enqueue(pcb_t* process) {
  if (!cfs_list) {
    cfs_list = process;
    process->next = 0;
    process->prev = 0;
    return;
  }
  
  // Insert sorted by vruntime (lowest first)
  pcb_t* current = cfs_list;
  while (current->next && current->vruntime < process->vruntime) {
    current = current->next;
  }
  
  if (current->vruntime >= process->vruntime) {
    // Insert before current
    process->next = current;
    process->prev = current->prev;
    if (current->prev) {
      current->prev->next = process;
    } else {
      cfs_list = process;
    }
    current->prev = process;
  } else {
    // Insert after current
    process->next = current->next;
    process->prev = current;
    if (current->next) {
      current->next->prev = process;
    }
    current->next = process;
  }
}

static void cfs_dequeue(pcb_t* process) {
  if (process->prev) {
    process->prev->next = process->next;
  } else {
    cfs_list = process->next;
  }
  if (process->next) {
    process->next->prev = process->prev;
  }
  process->next = 0;
  process->prev = 0;
}

static pcb_t* cfs_next(void) {
  if (!cfs_list) {
    return 0;
  }
  
  pcb_t* process = cfs_list;
  cfs_dequeue(process);
  return process;
}

void scheduler_enqueue(pcb_t* process) {
  if (!process) {
    return;
  }
  
  // Use MLFQ for interactive processes (high priority)
  // Use CFS for normal processes (fair scheduling)
  if (process->priority >= PRIORITY_HIGH) {
    mlfq_enqueue(process);
  } else {
    cfs_enqueue(process);
  }
}

void scheduler_dequeue(pcb_t* process) {
  if (!process) {
    return;
  }
  
  if (process->priority >= PRIORITY_HIGH) {
    mlfq_dequeue(process);
  } else {
    cfs_dequeue(process);
  }
}

// Get next process to run (hybrid MLFQ+CFS)
pcb_t* scheduler_next(void) {
  // First check MLFQ (high priority processes)
  pcb_t* next = mlfq_next();
  if (next) {
    return next;
  }
  
  // Then check CFS (normal processes)
  next = cfs_next();
  if (next) {
    return next;
  }
  
  // No processes ready, return kernel/idle
  return 0;
}

void scheduler_tick(void) {
  scheduler_ticks++;

  /* Wake any sleeping processes whose timer has expired */
  scheduler_wake_sleepers(scheduler_ticks);

  if (!current_running) {
      return;
  }
  
  pcb_t* process = current_running;
  
  // Update runtime
  process->total_runtime++;
  process->timeslice--;
  process->queue_ticks++;
  
  // Update CFS vruntime
  if (process->priority < PRIORITY_HIGH) {
    unsigned int weight = calculate_weight(process->priority);
    process->vruntime += calculate_vruntime_delta(1, weight);
  }
  
  // Check if timeslice expired
  if (process->timeslice <= 0) {
      if (process->priority >= PRIORITY_HIGH && process->queue_level < MLFQ_QUEUES - 1) {
        process->queue_level++;
    }
    
    
    if (process->priority >= PRIORITY_HIGH) {
      process->timeslice = calculate_timeslice(process->queue_level);
    } else {
      process->timeslice = CFS_TARGET_LATENCY;
    }

    // Reschedule
    scheduler_yield();
    return;
  }

  // MLFQ boost (prevent starvation)
  if (scheduler_ticks - last_boost >= MLFQ_BOOST_INTERVAL) {
    scheduler_boost();
    last_boost = scheduler_ticks;
  }
}

void scheduler_yield(void) {
  if (!current_running) {
      return;
  }
  
  pcb_t* current = current_running;
  current_running = 0;
  
  // Re-enqueue if still ready
  if (current->state == PROCESS_STATE_READY || current->state == PROCESS_STATE_RUNNING) {
    current->state = PROCESS_STATE_READY;
    scheduler_enqueue(current);
  }
  
  // Get next process
  pcb_t* next = scheduler_next();
  if (next) {
    next->state = PROCESS_STATE_RUNNING;
    current_running = next;
    context_switch(current, next);
  } else {
    // No processes ready, continue with current
    current->state = PROCESS_STATE_RUNNING;
    current_running = current;
  }
}

void scheduler_boost(void) {
  // Move all processes to highest queue
  for (int i = 0; i < MLFQ_QUEUES; i++) {
    while (mlfq_queues[i]) {
      pcb_t* process = mlfq_queues[i];
      mlfq_dequeue(process);
      process->queue_level = 0;
      process->queue_ticks = 0;
      process->timeslice = MLFQ_TIMESLICE_BASE;
      mlfq_enqueue(process);
    }
  }
}

unsigned int scheduler_get_load(void) {
  unsigned int total_processes = 0;
  for (int i = 0; i < MLFQ_QUEUES; i++) {
      total_processes += queue_sizes[i];
  }
  
  // Count CFS processes
  pcb_t* current = cfs_list;
  while (current) {
    total_processes++;
    current = current->next;
  }
  
  return (total_processes * 100) / MAX_PROCESSES;
}

void scheduler_set_current(pcb_t* process) {
  current_running = process;
}

/* --------------------------------------------------------------------------
 * Blocked process management
 * -------------------------------------------------------------------------- */

/* Block the current running process. It is removed from the run queue
 * and placed on the blocked list. The caller must set the process state
 * and sleep_until/waiting_for_pid fields BEFORE calling this. */
void scheduler_block_current(void) {
  if (!current_running) return;

  pcb_t* proc = current_running;
  proc->state = PROCESS_STATE_BLOCKED;

  /* Add to blocked list (simple singly-linked via next) */
  proc->next = blocked_list;
  blocked_list = proc;

  /* Clear current and schedule the next process */
  current_running = 0;

  pcb_t* next = scheduler_next();
  if (next) {
    next->state = PROCESS_STATE_RUNNING;
    current_running = next;
    context_switch(proc, next);
  } else {
    /* No other process -- restore current (shouldn't normally happen) */
    proc->state = PROCESS_STATE_RUNNING;
    current_running = proc;
  }
}

/* Check all sleeping processes and wake any whose timer has expired. */
void scheduler_wake_sleepers(unsigned int current_ticks) {
  pcb_t** pp = &blocked_list;
  while (*pp) {
    pcb_t* p = *pp;
    if (p->sleep_until > 0 && current_ticks >= p->sleep_until) {
      /* Remove from blocked list */
      *pp = p->next;
      p->next = 0;
      p->prev = 0;
      p->sleep_until = 0;
      p->state = PROCESS_STATE_READY;
      scheduler_enqueue(p);
    } else {
      pp = &((*pp)->next);
    }
  }
}

/* Wake a parent process that is waiting for a specific child PID. */
void scheduler_wake_parent(unsigned int child_pid) {
  pcb_t** pp = &blocked_list;
  while (*pp) {
    pcb_t* p = *pp;
    if (p->waiting_for_pid == child_pid) {
      /* Remove from blocked list */
      *pp = p->next;
      p->next = 0;
      p->prev = 0;
      p->waiting_for_pid = 0;
      p->state = PROCESS_STATE_READY;
      scheduler_enqueue(p);
      return; /* only one parent can wait */
    }
    pp = &((*pp)->next);
  }
}