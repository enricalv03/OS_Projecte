#include "process.h"
#include "scheduler.h"
#include "memory/vmm.h"
#include "memory/pmm.h"
#include "memory/heap.h"
#include "lib/kstring.h"
#include "sys/syscall.h"
#include "node.h"

/* The kernel's master page directory, defined in paging.asm (.bss, 4 KB).
 * It lives within the first 4 MB so its virtual address == physical address
 * (identity-mapped by setup_paging). */
extern unsigned int page_directory[1024];

static pcb_t process_table[MAX_PROCESSES];
static pcb_t* process_list = 0;
/* current_process is now the per-node current pointer from kernel_node_t.
 * Using a local macro alias keeps all existing reads/writes intact. */
#define current_process  (node_get_current_node()->current)
static unsigned int next_pid = 1;

unsigned int process_get_uid(void) {
  pcb_t* cur = process_get_current();
  return cur ? cur->uid : UID_ROOT;
}

void process_set_uid(unsigned int uid) {
  pcb_t* cur = process_get_current();
  if (!cur) return;
  /* Only root processes may switch UID, but allow the kernel shell
   * (PID 0) to change its own UID after a successful password check.
   * This keeps the security model intact for normal processes while
   * letting the interactive console use become admin / become user. */
  if (cur->pid != 0 && cur->uid != UID_ROOT)
    return;
  cur->uid = uid;
}

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
  /* The kernel process uses the kernel's own page directory.
   * Physical == virtual because it is within the identity-mapped first 4 MB. */
  kernel->page_directory = (unsigned int)page_directory;
  strcpy(kernel->name, "kernel");

  /* Start shell as normal user by default. Root/admin is only reachable
   * via the password-protected become admin command. */
  kernel->uid = UID_USER;

  /* Inherit the global default language. */
  kernel->language_id = i18n_get_default_lang();

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
  pcb->uid = current_process ? current_process->uid : UID_ROOT;
  /* Inherit the calling process's language, or the global default. */
  pcb->language_id = current_process ? current_process->language_id
                                      : i18n_get_default_lang();

  /* Allocate a fresh 4 KB page for this process's page directory.
   * We require it to fall within the identity-mapped first 4 MB so we can
   * write to it using its physical address as a virtual address. */
  unsigned int page_dir_phys = pmm_alloc_page();
  if (!page_dir_phys) {
      return 0;
  }
  if (page_dir_phys >= 0x400000u) {
      /* Fell outside the identity-mapped window — cannot safely write to it.
       * Free it and bail; this should not happen in normal early-boot usage. */
      pmm_free_page(page_dir_phys);
      return 0;
  }

  /* Clone the kernel page directory into the new page directory.
   * This ensures that kernel text/data/stack are always accessible from
   * user-mode page directories, which is required for syscall and interrupt
   * handlers to run after a CR3 switch. */
  unsigned int* new_pd     = (unsigned int*)page_dir_phys;
  unsigned int* kernel_pd  = page_directory;
  for (unsigned int i = 0; i < 1024; i++) {
      new_pd[i] = kernel_pd[i];
  }

  pcb->page_directory = page_dir_phys;

  /* Map the user stack into the process's own page directory.
   * vmm_alloc_and_map writes into whichever CR3 is currently loaded, so
   * we temporarily switch to the new PD, do the mappings, then restore the
   * kernel PD. */
  unsigned int saved_cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(saved_cr3));
  __asm__ volatile("mov %0, %%cr3" :: "r"(page_dir_phys) : "memory");

  /* User stack: 64 KB at 2 GB (0x80000000 – 0x8000FFFF). */
  pcb->stack_bottom = 0x80000000u;
  pcb->stack_top    = pcb->stack_bottom + 0x10000u;

  for (unsigned int i = 0; i < 16; i++) {
      unsigned int virt = pcb->stack_bottom + (i * 4096u);
      unsigned int phys = vmm_alloc_and_map(virt, VMM_USER_RW);
      if (!phys) {
          /* Restore kernel CR3 before tearing down (process_kill uses kernel
           * virtual addresses and must run with the kernel's page directory). */
          __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");
          process_kill(pcb->pid);
          return 0;
      }
  }

  /* Restore kernel page directory. */
  __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");

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

  /* Free stack pages only for user-mode processes (we allocated them via VMM).
   * Kernel threads use identity-mapped stacks and must not be unmapped. */
  if (pcb->stack_bottom && pcb->page_directory) {
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

/* Kernel .text is linked at 0x3000 (linker.ld). Valid entry points must be >= this. */
#define KERNEL_LOAD_ADDR  0x3000u

pcb_t* process_create_kernel_thread(const char* name, unsigned int priority, void (*entry_point)(void)) {
  unsigned int entry = (unsigned int)entry_point;
  if (entry < KERNEL_LOAD_ADDR) {
    /* Invalid/NULL entry would cause #UD at low EIP when we context-switch to this thread. */
    return 0;
  }

  pcb_t* pcb = find_free_pcb();
  if (!pcb)
  {
    return 0;
  }

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
  pcb->uid = current_process ? current_process->uid : UID_ROOT;
  pcb->language_id = current_process ? current_process->language_id
                                      : i18n_get_default_lang();

  /* Kernel threads share the kernel's page directory. */
  pcb->page_directory = (unsigned int)page_directory;

  /* Kernel threads use identity-mapped stacks. setup_paging (paging.asm)
   * maps the first 4 MB (0x00000000–0x003FFFFF). We put each thread’s
   * 16 KB stack at 2 MB + index*16 KB so no VMM calls are needed. */
  unsigned int index = (unsigned int)(pcb - process_table);
  pcb->stack_bottom = 0x00200000u + index * 0x4000u;
  pcb->stack_top    = pcb->stack_bottom + 0x4000u;

  pcb->esp = pcb->stack_top;
  pcb->eip = entry;
  pcb->eflags = 0x202;

  pcb->cs = 0x08;
  pcb->ds = 0x10;
  pcb->es = 0x10;
  pcb->fs = 0x10;
  pcb->gs = 0x10;
  pcb->ss = 0x10;

  pcb->next = process_list;
  if (process_list)
  {
    process_list->prev = pcb;
  }
  process_list = pcb;

  scheduler_enqueue(pcb);
  return pcb;
}

static void kthread_demo(void) {
  static const char msg[] = "[kthread_demo] hello from thread\n";

  /* Print once, then yield forever so the shell stays usable.
   * Avoids exercising the not-yet-complete kernel-thread exit path. */
  sys_write(1, (unsigned int)msg, sizeof(msg) - 1, 0);
  for (;;) {
    sys_yield(0, 0, 0, 0);
  }
}

void process_demo_spawn(void) {
  if (!process_create_kernel_thread("kthread_demo", PRIORITY_NORMAL, kthread_demo))
    return;
  /* Yield immediately so the new thread gets to run (kernel is not in the queue). */
  scheduler_yield();
}

unsigned int process_get_pid(void) {
  return current_process ? current_process->pid : 0;
}

/* ---- Signal delivery ---- */

int process_send_signal(unsigned int pid, unsigned int sig) {
  if (sig == 0 || sig >= NSIG) return -1;
  pcb_t* target = process_get_by_pid(pid);
  if (!target) return -1;

  /* If the signal is masked, still mark it as pending */
  target->pending_signals |= (1u << sig);

  /* For SIGKILL/SIGSTOP, deliver immediately regardless of mask */
  if (sig == SIGKILL) {
    target->pending_signals = 0;
    target->state = PROCESS_STATE_ZOMBIE;
    target->exit_code = 128 + SIGKILL;
    scheduler_wake_parent(target->pid);
    scheduler_dequeue(target);
  }
  return 0;
}

void process_check_signals(void) {
  pcb_t* cur = current_process;
  if (!cur || cur->pid == 0) return;
  if (!cur->pending_signals) return;

  for (unsigned int sig = 1; sig < NSIG; sig++) {
    unsigned int bit = 1u << sig;
    if (!(cur->pending_signals & bit)) continue;
    if (cur->signal_mask & bit) continue;   /* blocked */

    cur->pending_signals &= ~bit;

    unsigned int handler = cur->signal_handlers[sig];

    if (handler == SIG_IGN) continue;   /* ignore */

    if (handler == SIG_DFL) {
      /* Default actions */
      switch (sig) {
        case SIGCHLD:
        case SIGCONT:
          /* Default: ignore */
          break;
        case SIGSTOP:
          cur->state = PROCESS_STATE_BLOCKED;
          scheduler_yield();
          break;
        default:
          /* Default: terminate */
          cur->exit_code = 128 + (int)sig;
          cur->state = PROCESS_STATE_ZOMBIE;
          scheduler_wake_parent(cur->pid);
          scheduler_dequeue(cur);
          scheduler_yield();
          break;
      }
    }
    /* User-space handler: would need to push a sigframe onto the user stack.
     * Not implemented yet — treat as default until user-space signal trampolines
     * are wired up. */
  }
}

/* Immediately terminate a process (called via NODE_MSG_KILL from a remote CPU). */
void process_terminate(pcb_t* proc) {
    if (!proc || proc->state == PROCESS_STATE_DEAD) return;
    if (proc->state == PROCESS_STATE_READY) {
        scheduler_dequeue(proc);
    }
    proc->exit_code  = 128 + SIGKILL;
    proc->state      = PROCESS_STATE_ZOMBIE;
    scheduler_wake_parent(proc->pid);
}

void context_switch(pcb_t* from, pcb_t* to) {
  if (from) {
      from->state = PROCESS_STATE_READY;
  }

  to->state = PROCESS_STATE_RUNNING;
  current_process = to;

  /* Switch the page directory (CR3).  We always do this explicitly so that:
   *  - Switching to a kernel thread reloads the kernel PD (no-op if already
   *    loaded, but flushes the TLB cleanly).
   *  - Switching to a user process installs its own PD, giving it private
   *    virtual address space while still seeing the kernel's mappings. */
  if (to->page_directory != 0u) {
      __asm__ volatile(
          "mov %0, %%cr3"
          :
          : "r"(to->page_directory)
          : "memory"
      );
  }

  extern void context_switch_asm(pcb_t* from, pcb_t* to);
  context_switch_asm(from, to);
}

void context_save(pcb_t* pcb) {
  extern void context_save_asm(pcb_t* pcb);
  context_save_asm(pcb);
}