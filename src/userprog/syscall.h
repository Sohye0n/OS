#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"

void syscall_init (void);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_lime);
int wait (pid_t pid);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
int fibonacci(int n1);
int max_of_four_int(int n1, int n2, int n3, int n4);

/*prj4*/
int mmap(int fd, void* addr);

#endif /* userprog/syscall.h */
