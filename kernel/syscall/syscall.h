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

#define MAX_SYSCALLS    32

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

#endif