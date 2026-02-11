#include "process.h"
#include "scheduler.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../lib/kstring.h"

static pcb_t process_table[MAX_PROCESSES];
static pcb_t* process_list = 0;
static pcb_t* current_process = 0;
static unsigned int next_pid = 1;

void process_init(void) {
  // Clear process table
  memset(process_table, 0, sizeof(process_table));
  process_list = 0;
  current_process = 0;
  next_pid = 1;
  
  // Create kernel process (PID 0)
  pcb_t* kernel = &process_table[0];
  kernel->pid = 0;
  kernel->parent_pid = 0;
  kernel->state = PROCESS_STATE_RUNNING;
  kernel->priority = PRIORITY_REALTIME;
  kernel->base_priority = PRIORITY_REALTIME;
  kernel->page_directory = 0;
  strcpy(kernel->name, "kernel");
  
  current_process = kernel;
  process_list = kernel;
}

static pcb_t* find_free_pcb(void) {
  for (int i = 1; i < MAX_PROCESSES; i++)
  {
    if (process_table[i].pid == 0)
    {
      return &process_table[i];
    }
  }
  return 0;
}

pcb_t* process_create(const char* name, unsigned int priority, void (*entry_point)(void)) {
  pcb_t* pcb = find_free_pcb();
  if (!pcb) {
      return 0; // No free slots
  }
  
  // Initialize PCB
  memset(pcb, 0, sizeof(pcb_t));
  pcb->pid = next_pid++;
  pcb->parent_pid = current_process ? current_process->pid : 0;
  pcb->state = PROCESS_STATE_READY;
  pcb->priority = priority;
  pcb->base_priority = priority;
  pcb->queue_level = 0;
  pcb->queue_ticks = 0;
  pcb->vruntime = 0;
  pcb->timeslice = MLFQ_TIMESLICE_BASE;
  pcb->total_runtime = 0;
  
  strncpy(pcb->name, name, 31);
  pcb->name[31] = 0;

  // Allocate page directory for process
  unsigned int page_dir_phys = pmm_alloc_page();
  if (!page_dir_phys) {
      return 0; // Out of memory
  }
  
  // TODO: Copy kernel mappings and set up user space
  // For now, we'll use identity mapping
  pcb->page_directory = page_dir_phys;
  
  // Set up stack (allocate 64KB stack)
  pcb->stack_bottom = 0x80000000; // User stack at 2GB
  pcb->stack_top = pcb->stack_bottom + 0x10000; // 64KB stack
  
  // Allocate stack pages
  for (unsigned int i = 0; i < 16; i++) { // 16 pages = 64KB
      unsigned int virt = pcb->stack_bottom + (i * 4096);
      unsigned int phys = vmm_alloc_and_map(virt, VMM_USER_RW);
      if (!phys) {
          // Cleanup on failure
          process_kill(pcb->pid);
          return 0;
      }
  }

  // Set initial context
  pcb->esp = pcb->stack_top;
  pcb->eip = (unsigned int)entry_point;
  pcb->eflags = 0x202;
  pcb->cs = 0x1B;
  pcb->ds = 0x23;
  pcb->es = 0x23;
  pcb->fs = 0x23;
  pcb->gs = 0x23;
  pcb->ss = 0x23;
  
  // Add to process list
  pcb->next = process_list;
  if (process_list) {
      process_list->prev = pcb;
  }
  process_list = pcb;
  
  scheduler_enqueue(pcb);
  
  return pcb;
}

void process_exit(unsigned int exit_code) {
  if (!current_process || current_process->pid == 0) {
      return;  /* kernel process cannot exit */
  }

  current_process->state = PROCESS_STATE_ZOMBIE;
  current_process->exit_code = exit_code;

  /* Wake any parent blocked in sys_wait for this process */
  scheduler_wake_parent(current_process->pid);

  /* Remove from scheduler */
  scheduler_dequeue(current_process);

  /* Schedule next process */
  scheduler_yield();
}

void process_kill(unsigned int pid) {
  pcb_t* pcb = process_get_by_pid(pid);
  if (!pcb || pid == 0) {
      return;  /* can't kill kernel process */
  }

  /* Remove from scheduler */
  scheduler_dequeue(pcb);

  /* Free the allocated page directory */
  if (pcb->page_directory) {
      pmm_free_page(pcb->page_directory);
      pcb->page_directory = 0;
  }

  /* Free stack pages */
  if (pcb->stack_bottom) {
      for (unsigned int i = 0; i < 16; i++) {
          unsigned int virt = pcb->stack_bottom + (i * 4096);
          vmm_unmap_and_free(virt);
      }
  }

  /* Remove from process list */
  if (pcb->prev) {
      pcb->prev->next = pcb->next;
  } else {
      process_list = pcb->next;
  }
  if (pcb->next) {
      pcb->next->prev = pcb->prev;
  }

  /* Clear PCB */
  memset(pcb, 0, sizeof(pcb_t));
}

pcb_t* process_get_current(void) {
  return current_process;
}

pcb_t* process_get_by_pid(unsigned int pid) {
  pcb_t* current = process_list;
  while (current)
  {
    if (current->pid == pid)
    {
      return current;
    }
    current = current->next;
  }
  return 0;
}

unsigned int process_get_pid(void) {
  return current_process ? current_process->pid : 0;
}

void context_switch(pcb_t* from, pcb_t* to) {
  if (from) {
      from->state = PROCESS_STATE_READY;
  }
  
  to->state = PROCESS_STATE_RUNNING;
  current_process = to;
  
  // Switch page directory
  // TODO: Implement proper page directory switching
  
  // Call assembly context switch
  extern void context_switch_asm(pcb_t* from, pcb_t* to);
  context_switch_asm(from, to);
}

void context_save(pcb_t* pcb) {
  extern void context_save_asm(pcb_t* pcb);
  context_save_asm(pcb);
}