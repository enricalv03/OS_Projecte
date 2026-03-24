#include "syscall.h"
#include "fs/vfs.h"
#include "fs/elf.h"
#include "ipc/pipe.h"
#include "sched/process.h"
#include "sched/scheduler.h"
#include "memory/vmm.h"
#include "lib/kstring.h"
#include "i18n.h"

#include "arch.h"

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
    syscall_register(SYS_GETUID, sys_getuid);
    syscall_register(SYS_GETEUID, sys_geteuid);
    syscall_register(SYS_OPEN, sys_open);
    syscall_register(SYS_CLOSE, sys_close);
    syscall_register(SYS_SEEK, sys_seek);
    syscall_register(SYS_SET_LANGUAGE, sys_set_language);
    syscall_register(SYS_GET_LANGUAGE, sys_get_language);

    /* Phase 1E: extended POSIX surface */
    syscall_register(SYS_STAT,      sys_stat);
    syscall_register(SYS_FSTAT,     sys_fstat);
    syscall_register(SYS_DUP,       sys_dup);
    syscall_register(SYS_DUP2,      sys_dup2);
    syscall_register(SYS_PIPE,      sys_pipe);
    syscall_register(SYS_CHDIR,     sys_chdir);
    syscall_register(SYS_GETCWD,    sys_getcwd);
    syscall_register(SYS_BRK,       sys_brk);
    syscall_register(SYS_SIGACTION, sys_sigaction);

    /* Phase 4A */
    syscall_register(SYS_REDIRECT,  sys_redirect);
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

/* sys_exec -- Load and execute an ELF32 binary from the VFS.
 * arg1 = pointer to null-terminated path string (user virtual address)
 * arg2 = argv pointer (currently ignored)
 * Returns: -1 on error.  On success the process image is replaced and
 *          the scheduler will jump to the new entry point on next dispatch.
 */
int sys_exec(unsigned int filename_ptr, unsigned int argv_ptr,
             unsigned int arg3, unsigned int arg4) {
    (void)argv_ptr; (void)arg3; (void)arg4;

    if (filename_ptr == 0) return -1;

    pcb_t* cur = process_get_current();
    if (!cur || cur->pid == 0) return -1;

    const char* path = (const char*)filename_ptr;
    vfs_node_t* node = vfs_lookup(path);
    if (!node || node->type != VFS_NODE_FILE) return -1;

    unsigned int entry = 0;
    int rc = elf_load(node, cur, &entry);
    if (rc != 0) return -1;

    /* Replace the process image: set EIP to ELF entry, reset stack. */
    cur->eip = entry;
    cur->esp = cur->stack_top;
    cur->eflags = 0x202;

    /* The context restore on next scheduler dispatch will jump to entry. */
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

/* sys_kill -- Send a signal to a process.
 * arg1 = target PID, arg2 = signal number. */
int sys_kill(unsigned int pid, unsigned int signal,
             unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    if (pid == 0) return -1;
    return process_send_signal(pid, signal ? signal : SIGTERM);
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

    unsigned int now = arch_timer_get_ticks();
    current->sleep_until = now + (seconds * TICKS_PER_SECOND);

    /* Block current process -- scheduler will wake us */
    scheduler_block_current();

    return 0;
}

/* --------------------------------------------------------------------------
 * Console I/O system calls
 * -------------------------------------------------------------------------- */

void console_putchar_syscall(char c, unsigned char attr) {
    volatile unsigned char* vga = (volatile unsigned char*)(unsigned int)cons_cursor;
    *vga = (unsigned char)c;
    *(vga + 1) = attr;
    cons_cursor += 2;
}

void console_newline_syscall(void) {
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
 * fd>=2: write to open file at current offset.
 * Returns: number of bytes written, or -1 on error. */
int sys_write(unsigned int fd, unsigned int buf_ptr,
              unsigned int count, unsigned int arg4) {
    (void)arg4;
    if (buf_ptr == 0 || count == 0) return 0;

    const char* buf = (const char*)buf_ptr;

    /* fd >= 2: write to an open file */
    if (fd >= 2) {
        pcb_t* pcb = process_get_current();
        if (!pcb || fd >= MAX_OPEN_FILES || pcb->open_files[fd] == 0)
            return -1;
        vfs_node_t* node = (vfs_node_t*)pcb->open_files[fd];
        unsigned int off = pcb->open_file_offsets[fd];
        int n = vfs_write(node, off, count, buf);
        if (n > 0)
            pcb->open_file_offsets[fd] = off + (unsigned int)n;
        return n;
    }

    if (fd != 1 && fd != 2) return -1;  /* only stdout/stderr console */

    /* If stdout (fd 1) has been redirected to a file via sys_redirect,
     * write to the file instead of the console. */
    if (fd == 1) {
        pcb_t* pcb = process_get_current();
        if (pcb && pcb->open_files[1] != 0) {
            vfs_node_t* node = (vfs_node_t*)pcb->open_files[1];
            unsigned int off = pcb->open_file_offsets[1];
            int n = vfs_write(node, off, count, buf);
            if (n > 0)
                pcb->open_file_offsets[1] = off + (unsigned int)n;
            return n;
        }
    }

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
 * fd>=2: read from open file at current offset.
 * Returns: number of bytes read, or -1 on error. */
int sys_read(unsigned int fd, unsigned int buf_ptr,
             unsigned int count, unsigned int arg4) {
    (void)arg4;

    if (buf_ptr == 0 || count == 0) return 0;

    char* buf = (char*)buf_ptr;

    /* fd >= 2: read from open file */
    if (fd >= 2) {
        pcb_t* pcb = process_get_current();
        if (!pcb || fd >= MAX_OPEN_FILES || pcb->open_files[fd] == 0) {
            return -1;
        }
        vfs_node_t* node = (vfs_node_t*)pcb->open_files[fd];
        unsigned int off = pcb->open_file_offsets[fd];
        int n = vfs_read(node, off, count, buf);
        if (n > 0) {
            pcb->open_file_offsets[fd] = off + (unsigned int)n;
        }
        return (n >= 0) ? n : -1;
    }

    if (fd != 0) return -1;  /* only stdin and open files supported */

    /* fd == 0: stdin (keyboard buffer) */
    unsigned int read_count = 0;
    for (unsigned int i = 0; i < count; i++) {
        int ch = kbd_buffer_get();
        if (ch == -1) break;
        buf[i] = (char)ch;
        read_count++;
    }
    return (int)read_count;
}

int sys_getuid(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;

    pcb_t* current = process_get_current();
    return current ? (int)current->uid : 0;
}

int sys_geteuid(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;

    pcb_t* current = process_get_current();
    return current ? (int)current->uid : 0;
}

static pcb_t* syscall_current_pcb(void) {
    pcb_t* pcb = process_get_current();
    if (!pcb || pcb->pid == 0)
    {
        return 0;
    }
    return pcb;
}

int sys_open(unsigned int path_ptr, unsigned int flags, unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb || path_ptr == 0) return -1;

    const char* path = (const char*)path_ptr;
    vfs_node_t* node = vfs_lookup(path);

    if (!node) {
        /* Create the file if O_CREAT is set. */
        if (!(flags & O_CREAT)) return -1;
        node = vfs_create(path);
        if (!node) return -1;
    }

    if (node->type != VFS_NODE_FILE) return -1;

    /* O_TRUNC: zero the file contents before opening. */
    if (flags & O_TRUNC)
        vfs_truncate(node, 0);

    for (unsigned int fd = 2; fd < MAX_OPEN_FILES; fd++) {
        if (pcb->open_files[fd] == 0) {
            pcb->open_files[fd] = (void*)node;
            /* O_APPEND: start offset at end of file. */
            pcb->open_file_offsets[fd] = (flags & O_APPEND) ? node->size : 0;
            return (int)fd;
        }
    }
    return -1;
}

int sys_close(unsigned int fd, unsigned int arg2, unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb)
    {
        return -1;
    }
    if (fd >= MAX_OPEN_FILES)
    {
        return -1;
    }
    if (fd < 2)
    {
        return -1;
    }
    
    pcb->open_files[fd] = 0;
    pcb->open_file_offsets[fd] = 0;

    return 0;
}

/* sys_seek -- Set file offset. arg2 = offset, arg3 = whence (SEEK_SET/SEEK_CUR/SEEK_END). */
int sys_seek(unsigned int fd, unsigned int offset, unsigned int whence, unsigned int arg4) {
    (void)arg4;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;
    if (fd >= MAX_OPEN_FILES) return -1;
    if (fd < 2) return -1;
    vfs_node_t* node = (vfs_node_t*)pcb->open_files[fd];
    if (!node) return -1;

    unsigned int new_offset;
    if (whence == SEEK_SET) {
        new_offset = offset;
    } else if (whence == SEEK_CUR) {
        new_offset = pcb->open_file_offsets[fd] + offset;
    } else if (whence == SEEK_END) {
        new_offset = node->size + offset;
    } else {
        return -1;
    }
    pcb->open_file_offsets[fd] = new_offset;
    return (int)new_offset;
}

/* sys_set_language -- Change the calling process's language.
 * arg1 = language_id_t value.
 * Also updates the global default so new processes inherit the change.
 * Returns: 0 on success, -1 if lang_id is out of range. */
int sys_set_language(unsigned int lang_id, unsigned int arg2,
                     unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;
    if (lang_id >= (unsigned int)LANG_COUNT) return -1;

    pcb_t* cur = process_get_current();
    if (cur) cur->language_id = (language_id_t)lang_id;

    /* Update global default so new processes inherit this language. */
    i18n_set_default_lang((language_id_t)lang_id);
    return 0;
}

/* sys_get_language -- Return the calling process's current language_id. */
int sys_get_language(unsigned int arg1, unsigned int arg2,
                     unsigned int arg3, unsigned int arg4) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4;
    pcb_t* cur = process_get_current();
    if (cur) return (int)cur->language_id;
    return (int)i18n_get_default_lang();
}

/* ==========================================================================
 * Phase 1E: Extended POSIX syscall surface
 * ========================================================================== */

/* sys_stat -- stat a VFS path into a kstat_t */
int sys_stat(unsigned int path_ptr, unsigned int stat_ptr,
             unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    if (!path_ptr || !stat_ptr) return -1;
    vfs_node_t* node = vfs_lookup((const char*)path_ptr);
    if (!node) return -1;
    kstat_t* st = (kstat_t*)stat_ptr;
    st->st_size = node->size;
    st->st_type = (node->type == VFS_NODE_DIR) ? 1u : 0u;
    return 0;
}

/* sys_fstat -- stat an open file descriptor */
int sys_fstat(unsigned int fd, unsigned int stat_ptr,
              unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    if (!stat_ptr) return -1;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;
    if (fd >= MAX_OPEN_FILES || !pcb->open_files[fd]) return -1;
    vfs_node_t* node = (vfs_node_t*)pcb->open_files[fd];
    kstat_t* st = (kstat_t*)stat_ptr;
    st->st_size = node->size;
    st->st_type = (node->type == VFS_NODE_DIR) ? 1u : 0u;
    return 0;
}

/* sys_dup -- duplicate a file descriptor */
int sys_dup(unsigned int oldfd, unsigned int arg2,
            unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;
    if (oldfd >= MAX_OPEN_FILES || !pcb->open_files[oldfd]) return -1;
    for (unsigned int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (!pcb->open_files[fd]) {
            pcb->open_files[fd]        = pcb->open_files[oldfd];
            pcb->open_file_offsets[fd] = pcb->open_file_offsets[oldfd];
            return (int)fd;
        }
    }
    return -1;  /* EMFILE */
}

/* sys_dup2 -- duplicate oldfd as newfd.
 * newfd may be 0 (stdin) or 1 (stdout) to allow I/O redirection. */
int sys_dup2(unsigned int oldfd, unsigned int newfd,
             unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;
    if (oldfd >= MAX_OPEN_FILES || !pcb->open_files[oldfd]) return -1;
    if (newfd >= MAX_OPEN_FILES) return -1;
    if (oldfd == newfd) return (int)newfd;
    pcb->open_files[newfd]        = pcb->open_files[oldfd];
    pcb->open_file_offsets[newfd] = pcb->open_file_offsets[oldfd];
    return (int)newfd;
}

/* sys_pipe -- create an anonymous pipe.
 * arg1 = pointer to int[2] in caller's address space.
 * On return pipefd[0] = read end fd, pipefd[1] = write end fd.
 * Returns 0 on success, -1 on error. */
int sys_pipe(unsigned int pipefd_ptr, unsigned int arg2,
             unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;
    if (!pipefd_ptr) return -1;

    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;

    pipe_t* pipe = pipe_create();
    if (!pipe) return -1;

    /* Find two free file descriptor slots (≥ 2). */
    int rfd = -1, wfd = -1;
    for (unsigned int fd = 2; fd < MAX_OPEN_FILES; fd++) {
        if (pcb->open_files[fd] == 0) {
            if (rfd < 0) { rfd = (int)fd; }
            else if (wfd < 0) { wfd = (int)fd; break; }
        }
    }
    if (rfd < 0 || wfd < 0) { return -1; }

    pcb->open_files[rfd]         = (void*)&pipe->read_node;
    pcb->open_file_offsets[rfd]  = 0;
    pcb->open_files[wfd]         = (void*)&pipe->write_node;
    pcb->open_file_offsets[wfd]  = 0;

    int* pipefd = (int*)pipefd_ptr;
    pipefd[0] = rfd;
    pipefd[1] = wfd;
    return 0;
}

/* sys_chdir -- change working directory */
int sys_chdir(unsigned int path_ptr, unsigned int arg2,
              unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;
    if (!path_ptr) return -1;
    const char* path = (const char*)path_ptr;
    vfs_node_t* node = vfs_lookup(path);
    if (!node || node->type != VFS_NODE_DIR) return -1;
    vfs_set_cwd_with_path(node, path);
    return 0;
}

/* sys_getcwd -- copy current working directory path into user buffer */
int sys_getcwd(unsigned int buf_ptr, unsigned int size,
               unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    if (!buf_ptr || size == 0) return -1;
    const char* cwd = vfs_get_cwd_path();
    if (!cwd) return -1;
    char* buf = (char*)buf_ptr;
    unsigned int i = 0;
    while (cwd[i] && i < size - 1) {
        buf[i] = cwd[i];
        i++;
    }
    buf[i] = '\0';
    return (int)i;
}

/* sys_brk -- extend/query the process data segment.
 * arg1 = new_brk address (0 = query current).
 * Returns: current (or new) brk value.
 * Full mmap/brk implementation deferred to Phase 4 (needs per-process heap). */
int sys_brk(unsigned int new_brk, unsigned int arg2,
            unsigned int arg3, unsigned int arg4) {
    (void)arg2; (void)arg3; (void)arg4;
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;

    if (new_brk == 0) return (int)pcb->heap_end;  /* query */

    if (new_brk <= pcb->heap_start) return (int)pcb->heap_end;

    /* Map pages between current heap_end and new_brk */
    unsigned int cur = (pcb->heap_end + 0xFFFu) & ~0xFFFu;
    unsigned int end = (new_brk      + 0xFFFu) & ~0xFFFu;

    unsigned int saved_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved_cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(pcb->page_directory) : "memory");

    for (unsigned int va = cur; va < end; va += 4096) {
        if (!vmm_alloc_and_map(va, VMM_USER_RW)) {
            __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");
            return (int)pcb->heap_end;
        }
    }
    __asm__ volatile("mov %0, %%cr3" :: "r"(saved_cr3) : "memory");

    pcb->heap_end = new_brk;
    return (int)new_brk;
}

/* sys_sigaction -- set or query signal handler.
 * arg1 = signal number, arg2 = handler (SIG_DFL=0, SIG_IGN=1, or address).
 * Returns 0 on success, -1 on invalid signal. */
int sys_sigaction(unsigned int signum, unsigned int handler,
                  unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    if (signum == 0 || signum >= NSIG) return -1;
    if (signum == SIGKILL || signum == SIGSTOP) return -1;  /* cannot override */
    pcb_t* pcb = syscall_current_pcb();
    if (!pcb) return -1;
    pcb->signal_handlers[signum] = handler;
    return 0;
}

/* sys_redirect -- redirect stdout (fd 1) to a file.
 * arg1 = path (pointer to null-terminated string in caller's address space).
 * arg2 = mode: 0 = O_TRUNC (shell >), 1 = O_APPEND (shell >>).
 *
 * This is a convenience wrapper around open+dup2 so that the ASM shell
 * can redirect with a single INT 0x80, without needing C glue for dup2.
 *
 * Returns 0 on success, -1 on error. */
int sys_redirect(unsigned int path_ptr, unsigned int mode,
                 unsigned int arg3, unsigned int arg4) {
    (void)arg3; (void)arg4;
    if (path_ptr == 0) return -1;

    const char* path = (const char*)path_ptr;
    unsigned int flags = O_WRONLY | O_CREAT;
    if (mode == 1)
        flags |= O_APPEND;
    else
        flags |= O_TRUNC;

    int new_fd = sys_open(path_ptr, flags, 0, 0);
    if (new_fd < 0) return -1;

    /* dup2(new_fd, 1): make fd 1 point to the same node. */
    int rc = sys_dup2((unsigned int)new_fd, 1, 0, 0);
    sys_close((unsigned int)new_fd, 0, 0, 0);

    (void)path;
    return rc;
}