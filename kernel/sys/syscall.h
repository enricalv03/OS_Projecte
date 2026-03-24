#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_EXEC        3
#define SYS_WAIT        4
#define SYS_GETPID      5
#define SYS_GETPPID     6
#define SYS_KILL        7
#define SYS_YIELD       8
#define SYS_SLEEP       9
#define SYS_WRITE       10
#define SYS_READ        11
#define SYS_OPEN        12
#define SYS_CLOSE       13
#define SYS_SEEK        14
#define SYS_MMAP        15
#define SYS_MUNMAP      16
#define SYS_GETUID       17
#define SYS_GETEUID      18
#define SYS_SET_LANGUAGE 19   /* arg1 = language_id_t */
#define SYS_GET_LANGUAGE 20   /* returns current process's language_id */
#define SYS_STAT         21   /* arg1=path, arg2=stat_buf* */
#define SYS_FSTAT        22   /* arg1=fd, arg2=stat_buf* */
#define SYS_DUP          23   /* arg1=oldfd → new fd */
#define SYS_DUP2         24   /* arg1=oldfd, arg2=newfd */
#define SYS_PIPE         25   /* arg1=pipefd[2] */
#define SYS_CHDIR        26   /* arg1=path */
#define SYS_GETCWD       27   /* arg1=buf, arg2=size */
#define SYS_BRK          28   /* arg1=new_brk (0=query) */
#define SYS_SIGACTION    29   /* arg1=signum, arg2=handler */
#define SYS_REDIRECT     30   /* arg1=path, arg2=mode(0=trunc,1=append) — redirect stdout to file */

#define MAX_SYSCALLS    64
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

/* Flags for sys_open (arg2) */
#define O_RDONLY        0x00
#define O_WRONLY        0x01
#define O_RDWR          0x02
#define O_CREAT         0x40
#define O_TRUNC         0x200
#define O_APPEND        0x400

typedef int (*syscall_handler_t)(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);


void syscall_init(void);
void syscall_register(unsigned int num, syscall_handler_t handler);
int syscall_handle(unsigned int num, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);

int sys_exit(unsigned int exit_code, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_fork(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_exec(unsigned int filename_ptr, unsigned int argv_ptr, unsigned int arg3, unsigned int arg4);
int sys_wait(unsigned int pid, unsigned int status_ptr, unsigned int arg3, unsigned int arg4);
int sys_getpid(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_getppid(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_kill(unsigned int pid, unsigned int signal, unsigned int arg3, unsigned int arg4);
int sys_yield(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_sleep(unsigned int seconds, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_write(unsigned int fd, unsigned int buf_ptr, unsigned int count, unsigned int arg4);
int sys_read(unsigned int fd, unsigned int buf_ptr, unsigned int count, unsigned int arg4);

int sys_getuid(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_geteuid(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);

int sys_open(unsigned int path_ptr, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_close(unsigned int fd, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_seek(unsigned int fd, unsigned int offset, unsigned int whence, unsigned int arg4);
int sys_set_language(unsigned int lang_id, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_get_language(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);

/* Phase 1E: extended POSIX surface */
int sys_stat(unsigned int path_ptr, unsigned int stat_ptr, unsigned int arg3, unsigned int arg4);
int sys_fstat(unsigned int fd, unsigned int stat_ptr, unsigned int arg3, unsigned int arg4);
int sys_dup(unsigned int oldfd, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_dup2(unsigned int oldfd, unsigned int newfd, unsigned int arg3, unsigned int arg4);
int sys_pipe(unsigned int pipefd_ptr, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_chdir(unsigned int path_ptr, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_getcwd(unsigned int buf_ptr, unsigned int size, unsigned int arg3, unsigned int arg4);
int sys_brk(unsigned int new_brk, unsigned int arg2, unsigned int arg3, unsigned int arg4);
int sys_sigaction(unsigned int signum, unsigned int handler, unsigned int arg3, unsigned int arg4);

/* Phase 4A: redirect stdout to a file.
 * path: file path; mode: 0 = truncate (>), 1 = append (>>).
 * Returns 0 on success, -1 on error. */
int sys_redirect(unsigned int path_ptr, unsigned int mode, unsigned int arg3, unsigned int arg4);

/* Minimal stat structure (POSIX-compatible subset) */
typedef struct {
    unsigned int st_size;
    unsigned int st_type;   /* 0=file, 1=dir */
} kstat_t;

#endif