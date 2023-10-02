#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!\n");
  //printf("%s system call : %d\n ",thread_name(),*(uint32_t*)(f->esp));

  int esp=*(uint32_t*)(f->esp);
  //hex_dump(f->esp,f->esp,1000,1);

  //0부터 시작. echo x는 9.
  switch(esp){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if(is_user_vaddr(f->esp+4)) exit(*(uint32_t*)(f->esp+4));
      else exit(-1);
      break;
    case SYS_EXEC:
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax=exec((const char*)*(uint32_t*)(f->esp+4));
      break;
    case SYS_WAIT:   
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax=wait((pid_t)*(uint32_t*)(f->esp+4));
      break;
    case SYS_CREATE:
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      if(!is_user_vaddr(f->esp+20) || !is_user_vaddr(f->esp+24) || !is_user_vaddr(f->esp+28)) exit(-1);
      f->eax=read((int)*(uint32_t*)(f->esp+20),(void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
      break;
    case SYS_WRITE:
      if(!is_user_vaddr(f->esp+20) || !is_user_vaddr(f->esp+24) || !is_user_vaddr(f->esp+28)) exit(-1);
      f->eax=write((int)*(uint32_t*)(f->esp+20),(void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
      break; 
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
    case SYS_FIBONACCI:
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      else f->eax=fibonacci((int)*(uint32_t*)(f->esp+4));
      break;
    case SYS_MAX_OF_FOUR_INT:
      if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp+8) || !is_user_vaddr(f->esp+12) || !is_user_vaddr(f->esp+16)) exit(-1);
      f->eax=max_of_four_int((int)*(uint32_t*)(f->esp+4),(int)*(uint32_t*)(f->esp+8),(int)*(uint32_t*)(f->esp+12),(int)*(uint32_t*)(f->esp+16));
      break;
  }
}

void halt (void) 
{
  shutdown_power_off();
}

void exit (int status)
{
  printf("%s: exit(%d)\n",thread_name(),status);
  thread_current()->exit_status=status;
  //printf("cur thread name : %s\n",thread_current()->name);
  thread_exit();
}

pid_t exec (const char *file)
{
  //printf("exec : %s\n",file);
  tid_t child=process_execute(file);
  return (pid_t)child;
}

int wait (pid_t pid)
{
  return process_wait((tid_t)pid);
}

bool create (const char *file, unsigned initial_size)
{
  //return syscall2 (SYS_CREATE, file, initial_size);
}

bool remove (const char *file)
{
  //return syscall1 (SYS_REMOVE, file);
}

int open (const char *file)
{
  //return syscall1 (SYS_OPEN, file);
}

int filesize (int fd) 
{
  //return syscall1 (SYS_FILESIZE, fd);
}

int read (int fd, void *buffer, unsigned size)
{
  if(fd==0){
    for(int i=0; i<(int)size; i++){
      *(char*)(buffer+i)=input_getc();
    }
    return size;
  }
  return -1;
}

int write (int fd, const void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer)) exit(-1);
  if(fd==1){
    putbuf(buffer,size);
    return size;
  }
  return -1;
}

int fibonacci(int n1){
  //printf("fibonacci : %d\n",n1);
  int fib=-1,bf=1, bbf=1;
  if(n1==1 || n1==2) fib=1;
  else{
    for(int i=0; i<n1-2; i++){
      fib=bf+bbf;
      bbf=bf;
      bf=fib;
    }
  }
  return fib;
}

int max_of_four_int(int n1, int n2, int n3, int n4){
  int maxi;
  maxi=n1>n2?n1:n2;
  maxi=maxi>n3?maxi:n3;
  maxi=maxi>n4?maxi:n4;
  return maxi;
}

void seek (int fd, unsigned position) 
{
  //syscall2 (SYS_SEEK, fd, position);
}

unsigned tell (int fd) 
{
  //return syscall1 (SYS_TELL, fd);
}

void close (int fd)
{
  //syscall1 (SYS_CLOSE, fd);
}


