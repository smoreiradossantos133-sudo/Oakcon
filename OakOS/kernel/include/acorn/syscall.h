#ifndef ACORN_SYSCALL_H
#define ACORN_SYSCALL_H

enum syscall_number {
    SYSCALL_WRITE = 1,
    SYSCALL_GETPID = 2,
    SYSCALL_YIELD = 3,
    SYSCALL_EXIT = 4,
    SYSCALL_WAITPID = 5,
    SYSCALL_KILL = 6,
    SYSCALL_FORK = 7,
    SYSCALL_MKDIR = 8,
    SYSCALL_CREATE = 9,
    SYSCALL_WRITE_FILE = 10,
    SYSCALL_READ_FILE = 11,
};

long syscall_dispatch(unsigned long number, unsigned long arg0,
    unsigned long arg1, unsigned long arg2);
long acorn_syscall_dispatch_entry(unsigned long number, unsigned long arg0,
    unsigned long arg1, unsigned long arg2);
void acorn_user_exit(void);
int syscall_self_test(void);

#endif