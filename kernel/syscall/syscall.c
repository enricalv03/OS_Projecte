#include "syscall.h"
#include "../process/process.h"
#include "../process/scheduler.h"
#include "../lib/kstring.h"

/* PIT ticks for sleep timing (defined in pit.asm) */
extern unsigned int pit_get_ticks(void);

/* Console output for sys_write (defined in kernel.asm) */
extern unsigned int cons_cursor;
extern void scroll_up_one_row(void);

/* Keyboard buffer for sys_read */
extern int kbd_buffer_get(void);

/* System call handler table */
static syscall_handler_t syscall_table[MAX_SYSCALLS];

/* PIT runs at 100 Hz */
#define TICKS_PER_SECOND 100

/* --------------------------------------------------------------------------
 * Initialization
 * -------------------------------------------------------------------------- */

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));

    syscall_register(SYS_EXIT,    sys_exit);
    syscall_register(SYS_FORK,    sys_fork);
    syscall_register(SYS_EXEC,    sys_exec);
    syscall_register(SYS_WAIT,    sys_wait);
    syscall_register(SYS_GETPID,  sys_getpid);
    syscall_register(SYS_GETPPID, sys_getppid);
    syscall_register(SYS_KILL,    sys_kill);
    syscall_register(SYS_YIELD,   sys_yield);
    syscall_register(SYS_SLEEP,   sys_sleep);
    syscall_register(SYS_WRITE,   sys_write);
    syscall_register(SYS_READ,    sys_read);
}

void syscall_register(unsigned int num, syscall_handler_t handler) {
    if (num < MAX_SYSCALLS) {
        syscall_table[num] = handler;
    }
}

int syscall_handle(unsigned int num, unsigned int arg1, unsigned int arg2,
                   unsigned int arg3, unsigned int arg4) {
    if (num >= MAX_SYSCALLS || !syscall_table[num]) {
        return -1;
    }
    return syscall_table[num](arg1, arg2, arg3, arg4);
}

/* ==========================================================================
 * System call implementations
 * ========================================================================== */

/* sys_exit -- Terminate the calling process.
 * arg1 = exit_code */
int sys_exit(unsigned int exit_code, unsigned int arg2,
             unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;

    pcb_t* current = process_get_current();
    if (!current || current->pid == 0) {
        return -1;  /* kernel process cannot exit */
    }

    current->exit_code = exit_code;
    current->state = PROCESS_STATE_ZOMBIE;

    /* Wake any parent that is waiting on us */
    scheduler_wake_parent(current->pid);

    /* Remove from scheduler and yield */
    scheduler_dequeue(current);
    scheduler_yield();

    return 0; /* never reached */
}

/* sys_fork -- Create a new kernel thread.
 * In kernel mode (no user mode yet), fork creates a new process
 * that starts at the same instruction pointer with its own stack.
 * Returns: child PID in parent, 0 in child, -1 on error.
 *
 * NOTE: Since we are in kernel mode sharing the same address space,
 * this is really "create kernel thread" rather than a true fork. */
int sys_fork(unsigned int arg1, unsigned int arg2,
             unsigned int arg3, unsigned int arg4) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4;

    pcb_t* parent = process_get_current();
    if (!parent) return -1;

    /* For a proper fork we'd need to duplicate page directories, etc.
     * In our kernel-mode OS, we just create a child that shares the
     * same address space. The child will be enqueued but effectively
     * shares the kernel's flat mapping. */

    /* Create a child process with the parent's priority */
    pcb_t* child = process_create(parent->name, parent->priority, (void (*)(void))0);
    if (!child) return -1;

    /* Copy parent's register state into child */
    child->eax = 0;           /* fork returns 0 in child */
    child->ebx = parent->ebx;
    child->ecx = parent->ecx;
    child->edx = parent->edx;
    child->esi = parent->esi;
    child->edi = parent->edi;
    child->ebp = parent->ebp;
    child->eflags = parent->eflags;
    child->cs  = parent->cs;
    child->ds  = parent->ds;
    child->es  = parent->es;
    child->fs  = parent->fs;
    child->gs  = parent->gs;
    child->ss  = parent->ss;

    /* Return child PID to parent */
    return (int)child->pid;
}

/* sys_exec -- Load and execute a program.
 * In our kernel-mode OS, this sets the entry point of the current
 * process to a new function address.
 * arg1 = function pointer (cast to entry point)
 * Returns: -1 on error (on success, never returns to caller). */
int sys_exec(unsigned int entry_point, unsigned int argv_ptr,
             unsigned int arg3, unsigned int arg4) {
    (void)argv_ptr; (void)arg3; (void)arg4;

    if (entry_point == 0) return -1;

    pcb_t* current = process_get_current();
    if (!current || current->pid == 0) {
        return -1;  /* can't exec the kernel */
    }

    /* Set the new entry point */
    current->eip = entry_point;

    /* Reset stack to the top */
    current->esp = current->stack_top;

    /* The actual jump happens on context restore */
    return 0;
}

/* sys_wait -- Wait for a child process to exit.
 * arg1 = child PID (0 = any child)
 * arg2 = pointer to store exit status (can be 0)
 * Returns: PID of exited child, or -1 on error. */
int sys_wait(unsigned int pid, unsigned int status_ptr,
             unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;

    pcb_t* current = process_get_current();
    if (!current) return -1;

    /* If pid is specified, check if the child exists */
    if (pid != 0) {
        pcb_t* child = process_get_by_pid(pid);
        if (!child) return -1;

        /* If child is already a zombie, collect it immediately */
        if (child->state == PROCESS_STATE_ZOMBIE) {
            unsigned int exit_code = child->exit_code;
            if (status_ptr) {
                *((unsigned int*)status_ptr) = exit_code;
            }
            process_kill(pid);  /* clean up zombie */
            return (int)pid;
        }

        /* Otherwise, block until child exits */
        current->waiting_for_pid = pid;
        scheduler_block_current();

        /* We've been woken up -- the child should be a zombie now */
        child = process_get_by_pid(pid);
        if (child && child->state == PROCESS_STATE_ZOMBIE) {
            unsigned int exit_code = child->exit_code;
            if (status_ptr) {
                *((unsigned int*)status_ptr) = exit_code;
            }
            process_kill(pid);
            return (int)pid;
        }
    }

    return -1;
}

/* sys_getpid -- Return the calling process's PID. */
int sys_getpid(unsigned int arg1, unsigned int arg2,
               unsigned int arg3, unsigned int arg4) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4;
    return (int)process_get_pid();
}

/* sys_getppid -- Return the calling process's parent PID. */
int sys_getppid(unsigned int arg1, unsigned int arg2,
                unsigned int arg3, unsigned int arg4) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4;
    pcb_t* current = process_get_current();
    return current ? (int)current->parent_pid : 0;
}

/* sys_kill -- Send a signal to a process (simplified: just kills it). */
int sys_kill(unsigned int pid, unsigned int signal,
             unsigned int arg3, unsigned int arg4) {
    (void)signal; (void)arg3; (void)arg4;

    if (pid == 0) return -1;  /* can't kill kernel */
    pcb_t* target = process_get_by_pid(pid);
    if (!target) return -1;

    target->state = PROCESS_STATE_ZOMBIE;
    target->exit_code = 128 + 9;  /* killed by signal */
    scheduler_wake_parent(target->pid);
    scheduler_dequeue(target);
    return 0;
}

/* sys_yield -- Voluntarily give up the CPU. */
int sys_yield(unsigned int arg1, unsigned int arg2,
              unsigned int arg3, unsigned int arg4) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4;
    scheduler_yield();
    return 0;
}

/* sys_sleep -- Block the calling process for `seconds` seconds.
 * arg1 = number of seconds (minimum 1)
 * Uses PIT tick count at 100 Hz for timing. */
int sys_sleep(unsigned int seconds, unsigned int arg2,
              unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;

    if (seconds == 0) {
        scheduler_yield();
        return 0;
    }

    pcb_t* current = process_get_current();
    if (!current || current->pid == 0) {
        return -1;  /* kernel process shouldn't sleep */
    }

    unsigned int now = pit_get_ticks();
    current->sleep_until = now + (seconds * TICKS_PER_SECOND);

    /* Block current process -- scheduler will wake us */
    scheduler_block_current();

    return 0;
}

/* --------------------------------------------------------------------------
 * Console I/O system calls
 * -------------------------------------------------------------------------- */

static void console_putchar_syscall(char c, unsigned char attr) {
    volatile unsigned char* vga = (volatile unsigned char*)(unsigned int)cons_cursor;
    *vga = (unsigned char)c;
    *(vga + 1) = attr;
    cons_cursor += 2;
}

static void console_newline_syscall(void) {
    unsigned int offset = cons_cursor - 0xb8000;
    unsigned int row = offset / 160;
    row++;
    if (row >= 25) {
        scroll_up_one_row();
        row = 24;
    }
    cons_cursor = 0xb8000 + row * 160;
}

/* sys_write -- Write data to a file descriptor.
 * fd=1 (stdout): write to VGA console.
 * Returns: number of bytes written, or -1 on error. */
int sys_write(unsigned int fd, unsigned int buf_ptr,
              unsigned int count, unsigned int arg4) {
    (void)arg4;

    if (fd != 1) return -1;  /* only stdout supported */
    if (buf_ptr == 0 || count == 0) return 0;

    const char* buf = (const char*)buf_ptr;
    unsigned int written = 0;

    for (unsigned int i = 0; i < count; i++) {
        char c = buf[i];
        if (c == '\n') {
            console_newline_syscall();
        } else if (c == 0) {
            break;
        } else {
            console_putchar_syscall(c, 0x0F);
        }
        written++;
    }

    return (int)written;
}

/* sys_read -- Read data from a file descriptor.
 * fd=0 (stdin): read from keyboard buffer (non-blocking).
 * Returns: number of bytes read, or 0 if nothing available. */
int sys_read(unsigned int fd, unsigned int buf_ptr,
             unsigned int count, unsigned int arg4) {
    (void)arg4;

    if (fd != 0) return -1;  /* only stdin supported */
    if (buf_ptr == 0 || count == 0) return 0;

    char* buf = (char*)buf_ptr;
    unsigned int read_count = 0;

    for (unsigned int i = 0; i < count; i++) {
        int ch = kbd_buffer_get();
        if (ch == -1) break;  /* no more data */
        buf[i] = (char)ch;
        read_count++;
    }

    return (int)read_count;
}
