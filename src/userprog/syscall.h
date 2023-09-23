#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"

void syscall_init (void);
void sys_halt (void);
void sys_exit (int status);
pid_t sys_exec (const char *cmd_lime);
int sys_wait (pid_t pid);
int sys_read (int fd, void *buffer, unsigned size);
int sys_write (int fd, const void *buffer, unsigned size);

#endif /* userprog/syscall.h */
