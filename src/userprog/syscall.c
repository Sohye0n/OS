#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/page.h"
  
struct file{
  struct inode *inode;
  off_t pos;
  bool deny_write;
};
 
static void syscall_handler (struct intr_frame *);
struct lock file_lock;
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

void check_buffer(void* buffer, unsigned size){

  void* chk_addr=(buffer);
  //printf("check buffer!\n");
  struct vm_entry* vme;
  for(int i=0; i<size; i++){
    //printf("checking addr : %p\n",chk_addr+i*8);
    if(!is_user_vaddr(chk_addr+i*8)) exit(-1);
    if(chk_addr+i*8<(void*)0x08048000 || chk_addr+i*8>= (void*)0xc0000000) exit(-1);
    vme=search(chk_addr+i*8);
    if(vme==NULL) exit(-1);
    else if(vme->writable==false) exit(-1);
  }
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
      if(!is_user_vaddr(f->esp+4)||!is_user_vaddr(f->esp+8))exit(-1);
      f->eax=create((const char*)*(uint32_t*)(f->esp+4),(unsigned)*(uint32_t*)(f->esp+8));
      break;
    case SYS_REMOVE:
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax=remove((const char*)*(uint32_t*)(f->esp+4)); //const char*
      break;
    case SYS_OPEN:
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax=open((const char*)*(uint32_t*)(f->esp+4));
      break;
    case SYS_FILESIZE:
      if(!is_user_vaddr(f->esp+4))exit(-1);
      f->eax=filesize((int)*(uint32_t*)(f->esp+4));
      break;
    case SYS_READ:
      if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp+8) || !is_user_vaddr(f->esp+12)) exit(-1);
      //read 하려는 버퍼에 해당하는 vme가 존재하는지 체크
      //check_buffer((void*)*(uint32_t*)(f->esp+8),(unsigned)*(uint32_t*)(f->esp+12));
      f->eax=read((int)*(uint32_t*)(f->esp+4),(void*)*(uint32_t*)(f->esp+8),(unsigned)*(uint32_t*)(f->esp+12));
      break;
   case SYS_WRITE:
      if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp+8) || !is_user_vaddr(f->esp+12)) exit(-1);
      //write 하려는 버퍼에 해당하는 vme가 존재하는지 체크
      //check_buffer((void*)*(uint32_t*)(f->esp+8),(unsigned)*(uint32_t*)(f->esp+12));
      f->eax=write((int)*(uint32_t*)(f->esp+4),(void*)*(uint32_t*)(f->esp+8),(unsigned)*(uint32_t*)(f->esp+12));
      break; 
    case SYS_SEEK:
      if(!is_user_vaddr(f->esp+4)||!is_user_vaddr(f->esp+8))exit(-1);
      seek((int)*(uint32_t*)(f->esp+4),(unsigned)*(uint32_t*)(f->esp+8)); //int, unsigned
      break;
    case SYS_TELL:
      if(!is_user_vaddr(f->esp+4))exit(-1);
      f->eax=tell((int)*(uint32_t*)(f->esp+4));
      break;
    case SYS_CLOSE:
      if(!is_user_vaddr(f->esp+4))exit(-1);
      close((int)*(uint32_t*)(f->esp+4));
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
  //printf("%d  ",thread_current()->fdindex);
  //printf("%d  ",thread_current()->load_success);
  //현 프로세스의 열려있는 파일을 모두 close 해준다.
  struct file* f; 
  int lastindex=thread_current()->fdindex;
  for(int i=3; i<lastindex; i++){
    f=thread_current()->fd[i];
    if(f!=NULL) {
      //printf("%d  ",i);
      close(i);
      //file_allow_write(i);
    }
    //if(f==NULL)break;
  }
  //if(lock_held_by_current_thread(&file_lock)) lock_release(&file_lock);
  //이제 exit
  //printf("\n");
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
  if(file==NULL) exit(-1);
  return filesys_create(file,initial_size);
}

bool remove (const char *file)
{
  if(file==NULL) exit(-1);
  return filesys_remove(file);
}

int open (const char *file)
{
  //printf("syscall_open\n");
  if(file==NULL){
    exit(-1);
  }
  lock_acquire(&file_lock);
  struct file* f=filesys_open(file);
 
  if(f){
    //현 프로세스의 디스크립터 테이블에 저장
  
    //실행 중인 프로그램 파일이면 open은 해주나 write는 못하도록
    if(strcmp(file,thread_current()->name)==0) file_deny_write(f);

    //빈 자리를 찾는다
    //처음~마지막 중 close된 것이 있다면 그 자리에
    for(int i=3; i<thread_current()->fdindex; i++){
      if(thread_current()->fd[i]==NULL){
	thread_current()->fd[i]=f;
	//printf("fd : %d\n",i);
	lock_release(&file_lock);
	return i;
      }
    }
    //close된 파일이 없다면 제일 마지막 위치에
    thread_current()->fd[thread_current()->fdindex]=f;
    //printf("fd : %d\n",thread_current()->fdindex);
    lock_release(&file_lock);
    int ret=thread_current()->fdindex;
    thread_current()->fdindex+=1;
    return ret;

    //테이블이 다 찼으면
    return -1;
  }
      
  else{
    //파일 오픈에 실패하면 종료
    //printf("failed to open\n");
    lock_release(&file_lock);
    return -1;
  }
}

int filesize (int fd) 
{
  //정상적으로 파일을 열 수 없음
  if(fd<1 || fd>thread_current()->fdindex) exit(-1);
  //close된 파일
  if(fd>2 && thread_current()->fd[fd]==NULL) exit(-1);

  struct file* f=thread_current()->fd[fd];
  return file_length(f);
}

int read (int fd, void *buffer, unsigned size)
{

  //printf("sys_read. fd : %d size :%d\n buf addr: %p buf in: %s\n",fd,size,buffer,buffer);
  //정상적으로 파일을 열 수 없음
  if(fd>thread_current()->fdindex) return -1;
  //close된 파일
  if(fd>2 && thread_current()->fd[fd]==NULL) exit(-1);
  if(!is_user_vaddr(buffer)) exit(-1);
  
  lock_acquire(&file_lock);
  //STDIN
  if(fd==0){
    int i;
    for(i=0; i<(int)size; i++){
      if(*(char*)(buffer+i)=='\0') break;
      *(char*)(buffer+i)=input_getc();
    }
    lock_release(&file_lock);
    return i;
  }

  //file
  else{
    struct file* f=thread_current()->fd[fd];
      int ret= file_read(f,buffer,size);
      lock_release(&file_lock);
      return ret;
  }

}

int write (int fd, const void *buffer, unsigned size)
{
  //printf("buffer addr: %p buffer string:%s\n",buffer,buffer);
  //정상적인 파일디스크립터 번호가 아님
  if(fd<1 || fd>thread_current()->fdindex) return -1;
  //close된 파일
  if(fd>2 && thread_current()->fd[fd]==NULL) exit(-1);
  if(!is_user_vaddr(buffer))exit(-1);

  lock_acquire(&file_lock);

  //STDOUT 
  if(fd==1){
    //printf("putbuf start\n");
    putbuf(buffer,size);
    //printf("putbuf end\n");
    lock_release(&file_lock);
    return size;
  } 

  //file
  else{
    //우선 해당하는 파일이 존재하는지 찾는다
    struct file* f=thread_current()->fd[fd];
      //excutable file, running program에는 write할 수 없음.
      //running program -> thread_current()의 name과 현 file의 이름을 비교한다.
      //excutable file -> ??????

      //ppt를 다시 읽어보면...
      //excutable file을 삭제하는 것은 상관없음. 근데 핀토스에서는 현재 실행 중인 프로그램의 실행파일을 삭제하지 않겠다는 뜻. -> thread_current()의 name과 file을 비교하면 됨.
      //이건 open에서 비교해주자
      int ret=file_write(f,buffer,size);
      lock_release(&file_lock);
      return ret;
    }
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
  //정상적인 파일디스크립터 번호가 아님
  if(fd<1 || fd>thread_current()->fdindex) exit(-1);
  //close된 파일
  if(fd>2 && thread_current()->fd[fd]==NULL) exit(-1);

  file_seek(thread_current()->fd[fd],position);

}

unsigned tell (int fd) 
{
  
  //정상적인 파일디스크립터 번호가 아님
  if(fd<1 || fd>thread_current()->fdindex) exit(-1);
  //close된 파일
  if(fd>2 && thread_current()->fd[fd]==NULL) exit(-1);
  
  return (unsigned)file_tell(thread_current()->fd[fd]);
}

void close (int fd)
{
  //fd가 디스크립터 테이블에 존재했었나?
  if(thread_current()->fdindex<fd) exit(-1); //원래 리턴

  //fd는 현재 open 상태인가?
  if(thread_current()->fd[fd]){
    //close 전에 file_deny_write는 해제해줌
    file_allow_write(thread_current()->fd[fd]);
    //이제 close 해주자
    file_close(thread_current()->fd[fd]);
    //파일은 free되었지만 확실히 하기 위해 NULL
    thread_current()->fd[fd]=NULL;
    //close한 fd가 마지막 디스크립터였다면 
    if(fd==thread_current()->fdindex) thread_current()->fdindex-=1;
  }
  else exit(-1);
}



