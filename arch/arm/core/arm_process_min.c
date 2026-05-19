#include "sched/process.h"
#include "sched/scheduler.h"
#include "node.h"
#include "arch.h"

/*
 * Minimal ARM process table used during early bring-up.
 * Keeps scheduler integration functional before full ARM process port lands.
 */
#define ARM_MIN_PROCESSES 16
#define ARM_THREAD_STACK_WORDS 1024
static pcb_t arm_process_table[ARM_MIN_PROCESSES];
static unsigned int arm_thread_stacks[ARM_MIN_PROCESSES][ARM_THREAD_STACK_WORDS];
static unsigned int arm_next_pid = 1;

static void arm_reap_zombies(void) {
  for (unsigned int i = 1; i < ARM_MIN_PROCESSES; i++) {
    if (arm_process_table[i].state == PROCESS_STATE_ZOMBIE &&
        node_get_current_node()->current != &arm_process_table[i]) {
      arm_process_table[i].pid = 0;
      arm_process_table[i].state = PROCESS_STATE_DEAD;
      arm_process_table[i].next = 0;
      arm_process_table[i].prev = 0;
      arm_process_table[i].exit_code = 0;
      arm_process_table[i].stack_bottom = 0;
      arm_process_table[i].stack_top = 0;
      arm_process_table[i].esp = 0;
      arm_process_table[i].eip = 0;
    }
  }
}

void process_init(void) {
  for (unsigned int i = 0; i < ARM_MIN_PROCESSES; i++) {
    arm_process_table[i].pid = 0;
    arm_process_table[i].state = PROCESS_STATE_DEAD;
    arm_process_table[i].next = 0;
    arm_process_table[i].prev = 0;
  }

  /* PID 0: kernel bootstrap process */
  arm_process_table[0].pid = 0;
  arm_process_table[0].parent_pid = 0;
  arm_process_table[0].state = PROCESS_STATE_RUNNING;
  arm_process_table[0].priority = PRIORITY_REALTIME;
  arm_process_table[0].base_priority = PRIORITY_REALTIME;
  arm_process_table[0].next = 0;
  arm_process_table[0].prev = 0;

  arm_next_pid = 1;
  node_get_current_node()->current = &arm_process_table[0];
}

pcb_t* process_get_current(void) {
  return node_get_current_node()->current;
}

unsigned int process_get_pid(void) {
  pcb_t* cur = process_get_current();
  return cur ? cur->pid : 0;
}

pcb_t* process_get_by_pid(unsigned int pid) {
  for (unsigned int i = 0; i < ARM_MIN_PROCESSES; i++) {
    if (arm_process_table[i].state != PROCESS_STATE_DEAD &&
        arm_process_table[i].pid == pid) {
      return &arm_process_table[i];
    }
  }
  return 0;
}

void process_terminate(pcb_t* proc) {
  if (!proc) return;
  if (proc->pid == 0) return;

  if (proc->state == PROCESS_STATE_READY) {
    scheduler_dequeue(proc);
  }
  proc->state = PROCESS_STATE_ZOMBIE;
  proc->exit_code = 128u + SIGKILL;

  /* Early safety: if current process is terminated, fall back to PID 0. */
  if (node_get_current_node()->current == proc) {
    node_get_current_node()->current = &arm_process_table[0];
    arm_process_table[0].state = PROCESS_STATE_RUNNING;
  }
}

void process_exit(unsigned int exit_code) {
  pcb_t* cur = process_get_current();
  if (!cur) return;
  if (cur->pid == 0) return; /* bootstrap kernel process never exits */

  cur->exit_code = exit_code;
  cur->state = PROCESS_STATE_ZOMBIE;
  scheduler_yield();
  for (;;) { }
}

pcb_t* process_create_kernel_thread(const char* name, unsigned int priority, void (*entry_point)(void)) {
  if (!entry_point) return 0;
  arm_reap_zombies();

  unsigned int slot = ARM_MIN_PROCESSES;
  for (unsigned int i = 1; i < ARM_MIN_PROCESSES; i++) {
    if (arm_process_table[i].state == PROCESS_STATE_DEAD) {
      slot = i;
      break;
    }
  }
  if (slot == ARM_MIN_PROCESSES) {
    return 0;
  }

  pcb_t* pcb = &arm_process_table[slot];
  pcb->pid = arm_next_pid++;
  pcb->parent_pid = 0;
  pcb->state = PROCESS_STATE_READY;
  pcb->priority = priority;
  pcb->base_priority = priority;
  pcb->next = 0;
  pcb->prev = 0;
  pcb->exit_code = 0;

  if (name) {
    unsigned int j = 0;
    while (name[j] != 0 && j < sizeof(pcb->name) - 1) {
      pcb->name[j] = name[j];
      j++;
    }
    pcb->name[j] = 0;
  } else {
    pcb->name[0] = 0;
  }

  /* Build initial switch frame expected by context_switch_asm:
   * pop {r4-r11, pc}
   * 9 words = r4..r11 + pc
   */
  unsigned int* stack_top = &arm_thread_stacks[slot][ARM_THREAD_STACK_WORDS];
  unsigned int* frame = stack_top - 9;
  for (unsigned int i = 0; i < 8; i++) frame[i] = 0; /* r4-r11 */
  frame[8] = (unsigned int)entry_point;              /* pc */

  pcb->esp = (unsigned int)frame;
  pcb->eip = (unsigned int)entry_point;
  pcb->eflags = 0x13u; /* SVC mode, IRQ enabled */
  pcb->stack_bottom = (unsigned int)&arm_thread_stacks[slot][0];
  pcb->stack_top = (unsigned int)stack_top;

  scheduler_enqueue(pcb);
  return pcb;
}

void process_sleep_ticks(unsigned int ticks) {
  if (ticks == 0) {
    scheduler_yield();
    return;
  }

  pcb_t* cur = process_get_current();
  if (!cur) return;
  if (cur->pid == 0) return; /* bootstrap kernel process never sleeps */

  unsigned int now = arch_timer_get_ticks();
  scheduler_block_current_sleep_until(now + ticks);
}

unsigned int process_get_uid(void) {
  /* ARM bring-up: treat all kernel threads as root until user mode exists. */
  return UID_ROOT;
}

void process_set_uid(unsigned int uid) {
  (void)uid;
}

void process_kill(unsigned int pid) {
  pcb_t* p = process_get_by_pid(pid);
  if (!p) return;
  process_terminate(p);
}

int process_send_signal(unsigned int pid, unsigned int sig) {
  if (sig == 0 || sig >= NSIG) return -1;
  pcb_t* target = process_get_by_pid(pid);
  if (!target) return -1;

  target->pending_signals |= (1u << sig);
  if (sig == SIGKILL) {
    process_terminate(target);
  }
  return 0;
}

void process_check_signals(void) {
  /* ARM bring-up: user-mode signal delivery not implemented yet. */
}

void context_save(pcb_t* pcb) { (void)pcb; }
void context_restore(pcb_t* pcb) { (void)pcb; }