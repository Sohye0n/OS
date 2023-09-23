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
  //printf("esp : %d\n",*(uint32_t*)(f->esp));

  int esp=*(uint32_t*)(f->esp);
  //hex_dump(f->esp,f->esp,100,1);

  //0부터 시작. echo x는 9.
  switch(esp){
    case SYS_HALT:
      sys_halt();
      break;
    case SYS_EXIT:
      if(is_user_vaddr(f->esp+4)) sys_exit(*(uint32_t*)(f->esp+4));
      else sys_exit(-1);
      break;
    case SYS_EXEC:

      break;
    case SYS_WAIT:
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
      //참고 블로그에서는 모든 파라미터에 대해 is_user_vaddr을 검사하는데 굳이?라는 생각
      //버퍼는 이중포인터이기 때문에 검사해야하지만 나머지는 esp+x이기 때문에 안해도 될 듯?
      if(!is_user_vaddr(f->esp+24)) sys_exit(-1);
      sys_read((int)*(uint32_t*)(f->esp+20),(void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
      break;
    case SYS_WRITE:
      //참고 블로그에서는 모든 파라미터에 대해 is_user_vaddr을 검사하는데 굳이?라는 생각
      //버퍼는 이중포인터이기 때문에 검사해야하지만 나머지는 esp+x이기 때문에 안해도 될 듯?
      if(!is_user_vaddr(f->esp+24)) sys_exit(-1);
      sys_write((int)*(uint32_t*)(f->esp+20),(void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
  }
}

void sys_halt (void) 
{
  shutdown_power_off();
}

void sys_exit (int status)
{
  printf("%s: exit(%d)\n",thread_name(),status);
  thread_exit();
}

pid_t sys_exec (const char *file)
{
  tid_t child=process_execute(file);
  return (pid_t)child;
}

int sys_wait (pid_t pid)
{
  //return syscall1 (SYS_WAIT, pid);
}

bool sys_create (const char *file, unsigned initial_size)
{
  //return syscall2 (SYS_CREATE, file, initial_size);
}

bool sys_remove (const char *file)
{
  //return syscall1 (SYS_REMOVE, file);
}

int sys_open (const char *file)
{
  //return syscall1 (SYS_OPEN, file);
}

int sys_filesize (int fd) 
{
  //return syscall1 (SYS_FILESIZE, fd);
}

int sys_read (int fd, void *buffer, unsigned size)
{
  for(int i=0; i<size; i++) *(char*)(buffer+i)=input_getc();
  return size;
}

int sys_write (int fd, const void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer)) sys_exit(-1);
  putbuf(buffer,size);
  return size;
}

void sys_seek (int fd, unsigned position) 
{
  //syscall2 (SYS_SEEK, fd, position);
}

unsigned sys_tell (int fd) 
{
  //return syscall1 (SYS_TELL, fd);
}

void sys_close (int fd)
{
  //syscall1 (SYS_CLOSE, fd);
}


