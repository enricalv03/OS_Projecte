#ifndef PROCESS_H
#define PROCESS_H

#include "memory/vmm.h"
#include "i18n.h"

#define PROCESS_STATE_RUNNING   0
#define PROCESS_STATE_READY     1
#define PROCESS_STATE_BLOCKED   2
#define PROCESS_STATE_ZOMBIE    3
#define PROCESS_STATE_DEAD      4

/* ---- POSIX-like signal numbers ---- */
#define SIGHUP     1
#define SIGINT     2
#define SIGQUIT    3
#define SIGILL     4
#define SIGFPE     8
#define SIGKILL    9
#define SIGSEGV   11
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19

#define NSIG      32   /* total number of signal slots */

/* Signal disposition (per-process) */
#define SIG_DFL   0    /* default action */
#define SIG_IGN   1    /* ignore signal */

#define PRIORITY_REALTIME     10
#define PRIORITY_HIGH         7
#define PRIORITY_NORMAL       5
#define PRIORITY_LOW          3
#define PRIORITY_IDLE         1

#define UID_ROOT              0
#define UID_USER              1

#define MAX_PROCESSES       256
#define MAX_OPEN_FILES       16

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

  unsigned int preempted_from_irq;  /* 1 if preempted from timer IRQ (needs iret restore) */

  char name[32];
  unsigned int exit_code;
  unsigned int uid;

  /* Sleep / wait support */
  unsigned int sleep_until;       /* PIT tick to wake up at (0 = not sleeping) */
  unsigned int waiting_for_pid;   /* PID we're waiting on (0 = not waiting) */

  /* Open file table: fd 0=stdin, 1=stdout, 2.. = vfs_node_t* (stored as void*). */
  void* open_files[MAX_OPEN_FILES];
  unsigned int open_file_offsets[MAX_OPEN_FILES];

  /* Per-process language context.  Initialised to the global default; can be
   * changed at runtime via sys_set_language().  Kernel error messages and
   * shell output are formatted in this language when addressed to this process. */
  language_id_t language_id;

  /* ---- Signal support ---- */
  unsigned int pending_signals;   /* bitmask of pending signals (bit n = signal n) */
  unsigned int signal_mask;       /* bitmask of blocked signals */
  /* disposition: SIG_DFL=0, SIG_IGN=1, or user handler address */
  unsigned int signal_handlers[NSIG];

  struct process_control_block* next;
  struct process_control_block* prev;
} pcb_t;

void process_init(void);
pcb_t* process_create(const char* name, unsigned int priority, void (*entry_point)(void));
void process_exit(unsigned int exit_code);
void process_kill(unsigned int pid);
pcb_t* process_get_current(void);
pcb_t* process_get_by_pid(unsigned int pid);
pcb_t* process_create_kernel_thread(const char* name, unsigned int priority, void (*entry_point)(void));
unsigned int process_get_pid(void);
unsigned int process_get_uid(void);
void process_set_uid(unsigned int uid);
void process_demo_spawn(void);
void context_switch(pcb_t* from, pcb_t* to);
void context_save(pcb_t* pcb);
void context_restore(pcb_t* pcb);

/* Send signal `sig` to process `pid`.
 * Returns 0 on success, -1 if process not found or sig invalid. */
int process_send_signal(unsigned int pid, unsigned int sig);

/* Check and deliver pending signals for the current process.
 * Called by the scheduler before returning to user mode. */
void process_check_signals(void);

/* Immediately terminate a process and free its resources.
 * Called from scheduler_process_messages() on NODE_MSG_KILL. */
void process_terminate(pcb_t* proc);

#endif
