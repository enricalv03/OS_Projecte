#ifndef PROCESS_H
#define PROCESS_H

#include "../memory/vmm.h"

#define PROCESS_STATE_RUNNING   0
#define PROCESS_STATE_READY     1
#define PROCESS_STATE_BLOCKED   2
#define PROCESS_STATE_ZOMBIE    3
#define PROCESS_STATE_DEAD      4

#define PRIORITY_REALTIME     10
#define PRIORITY_HIGH         7
#define PRIORITY_NORMAL       5
#define PRIORITY_LOW          3
#define PRIORITY_IDLE         1

#define MAX_PROCESSES       256

typedef struct process_control_block {
  unsigned int pid;
  unsigned int parent_pid;
  unsigned int state;
  unsigned int priority;
  unsigned int base_priority;

  unsigned int eax, ebx, ecx, edx;
  unsigned int esi, edi, ebp, esp;
  unsigned int eip;
  unsigned int eflags;
  unsigned int cs, ds, es, fs, gs, ss;

  unsigned int page_directory;
  unsigned int heap_start;
  unsigned int heap_end;
  unsigned int stack_top;
  unsigned int stack_bottom;

  unsigned int vruntime;
  unsigned int timeslice;
  unsigned int total_runtime;

  unsigned int queue_level;
  unsigned int queue_ticks;

  char name[32];
  unsigned int exit_code;

  /* Sleep / wait support */
  unsigned int sleep_until;       /* PIT tick to wake up at (0 = not sleeping) */
  unsigned int waiting_for_pid;   /* PID we're waiting on (0 = not waiting) */

  struct process_control_block* next;
  struct process_control_block* prev;
} pcb_t;

void process_init(void);
pcb_t* process_create(const char* name, unsigned int priority, void (*entry_point)(void));
void process_exit(unsigned int exit_code);
void process_kill(unsigned int pid);
pcb_t* process_get_current(void);
pcb_t* process_get_by_pid(unsigned int pid);
unsigned int process_get_pid(void);

void context_switch(pcb_t* from, pcb_t* to);
void context_save(pcb_t* pcb);
void context_restore(pcb_t* pcb);

#endif
