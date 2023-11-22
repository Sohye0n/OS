#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/page.h"


static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);
void get_filename(const char*, char*);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;
  char file_name_parsed[256];
  get_filename(file_name,file_name_parsed);

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);
  //printf("1\n");
  if(filesys_open(file_name_parsed)==NULL) return -1;
  //printf("2\n");
  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name_parsed, PRI_DEFAULT, start_process, fn_copy);
  //printf("process %s executing... pid : %d\n",file_name,tid);  
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy);

  /*로드에 실패한 프로세스는 id가 -1*/
  struct list_elem* index_key=list_begin(&thread_current()->child_list);
  struct list_elem* last_key=list_end(&thread_current()->child_list);
  struct thread* child;
  struct thread* load_fail=NULL;

  //자식 프로세스가 완전히 load 될 때까지 기다리기
  while(1){
    child=list_entry(index_key,struct thread,Iamyourchild);
    //printf("%d  ",child->tid);
    if(child->tid==tid){ //exit_status도 체크하면 무한루프 걸림. 자식load()보다 부모가 먼저 실행될 수도.
      load_fail=child;
      break;
    }
    if(index_key==last_key) break;
    index_key=list_next(index_key);
  }
  //자식 프로세스의 load를 기다리기
  if(load_fail)sema_down(&load_fail->load_lock);

  //load에 실패한 것이 있다면 얘를 완전히 제거해주자
  //process_wait에서 얘의 exit_status인 -1을 리턴해줌.
  if(load_fail->load_success==0){
    return process_wait(tid);
  }

  return tid;
}



/*입력된 문자열에서 파일 이름만 가져오는 함수*/
void get_filename(const char* org, char* result){
  //띄어쓰기 단위로 단어를 분리하면 됨.
  int i=0;
  //파일 이름 뒤에는 널문자나 띄어쓰기가 올 것임.
  while(org[i]!='\0'&&org[i]!=' '){
    result[i]=org[i];
    i++;
  }
  //result의 마지막에도 null을 넣어줘야 함.
  result[i]='\0';
}

//전체 입력의 개수를 리턴한다
int get_arguments(char* org, char** result){
  //공백 단위로 단어를 분리하면 됨.
  //스레드 호출 함수이므로 strtok_r을 사용해야 함.
  char* ret_ptr, *next_ptr;
  int i=0,cnt=0;
  ret_ptr=strtok_r(org," ",&next_ptr);
  while(ret_ptr){
    cnt++;
    result[i++]=ret_ptr;
    ret_ptr=strtok_r(NULL," ",&next_ptr); 
  } 
  //마지막에는 null문자 넣어주기
  result[i]="\0";
  return cnt;
}

// //파싱된 입력을 토대로 스택에 집어넣는다
// void push(char* cmd, void**esp){
//   char** cmd_parsed=malloc(sizeof(char*)*256); //4kb
//   char* cmd_copy=malloc(sizeof(char)*256); //4kb

//   strlcpy(cmd_copy,cmd,strlen(cmd)+1);
//   //printf("%c\n",cmd[1]);
//   int cmd_num=get_arguments(cmd_copy,cmd_parsed);
//   char* esp_copy=*esp;
//   //printf("copy : %p, org : %p\n",esp_copy,*esp);
//   //printf("parsing end. argc : %d\n",cmd_num);
  
//   //명령어들부터 집어넣는다.
//   //오른쪽 끝부터 먼저 들어간다.
//   int tmp,cmd_length=0;
//   for(int i=cmd_num-1; i>=0; i--){
//     tmp=strlen(cmd_parsed[i])+1; //null문자 포함
//     cmd_length+=tmp;
//     *esp=*esp-tmp;
//     strlcpy(*esp,cmd_parsed[i],tmp);
//     //printf("address : %p | token : %s\n",*esp,cmd_parsed[i]);
//   }

//   //집어넣은 명령어의 총 길이가 4의 배수인지 확인한다(워드사이즈 32비트)
//   *esp=*esp-4+(cmd_length%4);
//   //printf("addresss : %p\n",*esp);

//   //명령어들의 주소를 집어넣는다.
//   //null문자부터 먼저 집어넣는다.
//   *esp=*esp-4; 
//   **(uint32_t **)esp=0;
//   //printf("addresss : %p\n",*esp);

//   //4개의 명령어의 주소값을 집어넣는다.
//   for(int i=cmd_num-1; i>=0; i--){
//     *esp=*esp-4;
//     esp_copy-=( strlen(cmd_parsed[i]) + 1 );
//     **(uint32_t **)esp=esp_copy;
//     //printf("addresss : %p | data : %0x\n",*esp,**(uint32_t **)esp);
//   } 
 
//   //argv배열의 주소값을 집어넣는다.
//   *esp-=4;
//   **(uint32_t**)esp=*esp+4;
//   //printf("addresss : %p | data : %0x\n",*esp,**(uint32_t **)esp);

//   //argc를 넣는다
//   *esp-=4;
//   **(uint32_t**)esp=cmd_num;
//   //printf("addresss : %p | data : %d\n",*esp,**(uint32_t **)esp);

//   //return address를 넣어준다.
//   *esp-=4;
//   **(uint32_t **)esp=0;
//   //printf("addresss : %p\n",*esp);

//   //hex_dump(*esp,*esp,100,1);
//   free(cmd_copy);
//   free(cmd_parsed);
//   //hex_dump(*esp,*esp,100,1);
// }



/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  /*prj4*/
  //load 하기 전에 해시테이블 생성
  vm_init(&thread_current()->vm_hash);
  /*prj4*/

  success = load (file_name, &if_.eip, &if_.esp); //입력으로 받은 프로그램이 실행 가능한지 체크 후 스택을 할당해줌.
  
  /* If load failed, quit. */
  palloc_free_page (file_name);
  if(!success) thread_current()->load_success=0;
  sema_up(&thread_current()->load_lock);  
  if (!success){ 
    exit(-1);
  }

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid) 
{
  // int i;
  // for (i = 0; i < 1000000000; i++);
  // return -1;

  /*prj1에서 추가한 내용*/
  struct thread* mychild; //자식 프로세스를 저장할 포인터
  struct thread* me=thread_current(); //현재 실행 중인 프로세스 = 나
  struct list_elem* mychild_key; //자식 프로세스를 찾아올 키
  struct list_elem* last_key=list_end(&me->child_list);
  int exit_status;
  //printf("process_wait start. I'm %d(%s), waiting for %d\n",me->tid,me->name,child_tid);

  //부모 프로세스의 자식 프로세스 키들을 하나씩 가져옴
  //가져온 자식 프로세스 키를 통해 전체 프로세스 리스트에서 자식 프로세스의 몸통을 가져옴
  //파라미터로 받은 tid와 이 자식프로세스의 tid를 비교.
  mychild_key=list_begin(&me->child_list); //부모 프로세스의 첫번째 자식 프로세스 키
  while(1){
    mychild=list_entry(mychild_key,struct thread,Iamyourchild); //키로 프로세스 몸통 찾기 //부모에서가 아니라 전체 프로세스에서 찾음
    
    if(mychild->tid==child_tid){ //일치한다면
      //printf("1 my tid:%d child tid:%d\n",me->tid,mychild->tid);
      sema_down(&mychild->state); //세마포어 1 줄임 (자식 종료까지 기다림)
      //printf("2 my tid:%d child tid:%d\n",me->tid,mychild->tid);
      list_remove(mychild_key); //자식 리스트에서 제거
      //printf("3 my tid:%d child tid:%d\n",me->tid,mychild->tid);
      exit_status=mychild->exit_status; //자식의 exit_status 저장
      sema_up(&mychild->mem);
      //printf("parent %d process_wait end for child %d\, exit_status : %d\n",me->tid,child_tid,exit_status);
      return exit_status;
    }

    if(mychild_key==last_key) break;
    mychild_key=list_next(mychild_key); //일치하지 않는다면 다음 원소를 탐색
  }

  return -1;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL)
    {  
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
      /*prj4*/
      //프로세스의 vm 해시테이블과 그 안의 vm_entry들을 다 free 해줘야 한다.
      vm_delete(&thread_current()->vm_hash);
      /*prj4*/
      //printf("cur thread : %s, sema up!\n",cur->name); 
    }
    sema_up(&cur->state); 
    sema_down(&cur->mem);
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/*prj4*/
bool handle_mm_fault(void* vaddr){
  return true;
  //create an empty frame for physical memory
  struct page *frame=palloc_get_page(PAL_USER);
  
  //if created
  if(frame){
    //load physical memory and map
    return true;
  }
  else return false;
}
/*prj4*/

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /*파일 이름을 파싱해줌*/
  char parsed_file_name[256]; //스택은 최대 4kb
  get_filename(file_name,parsed_file_name);
  /* Open executable file. */
  file = filesys_open (parsed_file_name);

  //printf("%d %d\n",strlen(file_name),(file_name[12]=='\0'));
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file)){
        //printf ("load: %s: 1\n", file_name);
        goto done;
      }
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr){
        //printf ("load: %s: 2\n", file_name);
        goto done;
      }
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable)){
                                  //printf ("load: %s: 3\n", file_name);
                goto done;
              }
            }
          else{
            //printf ("load: %s: 4\n", file_name);
            goto done;
          }
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;

  char* cmd_parsed[256]; //4kb
  char cmd_copy[256]; //4kb

  strlcpy(cmd_copy,file_name,strlen(file_name)+1);
  //printf("%c\n",cmd[1]);
  int cmd_num=get_arguments(cmd_copy,cmd_parsed);
  char* esp_copy=*esp;
  //printf("copy : %p, org : %p\n",esp_copy,*esp);
  //printf("parsing end. argc : %d\n",cmd_num);
  
  //명령어들부터 집어넣는다.
  //오른쪽 끝부터 먼저 들어간다.
  int tmp,cmd_length=0;
  for(int i=cmd_num-1; i>=0; i--){
    tmp=strlen(cmd_parsed[i])+1; //null문자 포함
    cmd_length+=tmp;
    *esp=*esp-tmp;
    strlcpy(*esp,cmd_parsed[i],tmp);
    //printf("address : %p | token : %s\n",*esp,cmd_parsed[i]);
  }

  //집어넣은 명령어의 총 길이가 4의 배수인지 확인한다(워드사이즈 32비트)
  *esp=*esp-4+(cmd_length%4);
  //printf("addresss : %p\n",*esp);

  //명령어들의 주소를 집어넣는다.
  //null문자부터 먼저 집어넣는다.
  *esp=*esp-4; 
  **(uint32_t **)esp=0;
  printf("addresss : %p\n",*esp);

  //4개의 명령어의 주소값을 집어넣는다.
  for(int i=cmd_num-1; i>=0; i--){
    *esp=*esp-4;
    esp_copy-=( strlen(cmd_parsed[i]) + 1 );
    **(uint32_t **)esp=esp_copy;
    //printf("addresss : %p | data : %0x\n",*esp,**(uint32_t **)esp);
  }

    //argv배열의 주소값을 집어넣는다.
    *esp-=4;
    **(uint32_t**)esp=*esp+4;
    //printf("addresss : %p | data : %0x\n",*esp,**(uint32_t **)esp);

    //argc를 넣는다
    *esp-=4;
    **(uint32_t**)esp=cmd_num;
    //printf("addresss : %p | data : %d\n",*esp,**(uint32_t **)esp);

    //return address를 넣어준다.
    *esp-=4;
    **(uint32_t **)esp=0;
    //printf("addresss : %p\n",*esp);

    //hex_dump(*esp,*esp,100,1);
    //hex_dump(*esp,*esp,100,1);
  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  
  //!!여기 수정함!!//
  if (phdr->p_vaddr < PGSIZE) //원본
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      //다이렉트로 물리페이지와 매핑하는게 아니라 vme와 매핑한다.
      //make vm_entry
      struct vm_entry* vm_entry=malloc(sizeof(vm_entry));
      vm_entry->type=BIN;
      vm_entry->vaddr=upage;
      vm_entry->writable=writable;
      vm_entry->is_loaded=false; //아직 물리 메모리에 올라가진 않았으니
      vm_entry->offset=ofs;
      vm_entry->read_bytes=page_read_bytes;
      vm_entry->zero_bytes=page_zero_bytes;

      //put hash_elem in the hashtable
      hash_insert(&thread_current()->vm_hash,&vm_entry->hash_elem);

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free_page (kpage);
    }

    //이 스택(PCB)에 대한 vm entry를 생성함.
    struct vm_entry* vm_entry=malloc(sizeof(vm_entry));
    vm_entry->type=0; //일단 모르니까 냅두기
    vm_entry->vaddr=(uint8_t *)PHYS_BASE-PGSIZE;
    vm_entry->writable=true; //스택에 쌓임 = 수정 가능함
    vm_entry->is_loaded=true; //스택에 쌓임 = 메모리에 올라옴

    //put hash_elem in the hashtable
    hash_insert(&thread_current()->vm_hash,&vm_entry->hash_elem);
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

/*prj4*/
bool stack_growth(void* vaddr){
  //palloc_get_page로 페이지를 받는다. 이 페이지는 물리 메모리와 매핑 o.
  //유저 메모리 풀에 할당해야 하므로
  void* new_page=palloc_get_page(PAL_USER);
  //유저 메모리 풀이 다 찼다면? -> swap 해와야함. 지금은 일단 어쩔수없지
  if(new_page==NULL) return false;

  //유저 메모리 풀에 공간이 있다면? -> 이 new_page를 리턴해야.
  struct vm_entry* vme=malloc(sizeof(struct vm_entry));
  vme->vaddr=pg_round_down(vaddr);
  //vme->type=VM_SWAP; <<잘 모르니까 일단 패스
  //물리 메모리에 올라왔으니
  vme->is_loaded=true;
  //왜인진 모르겠으나..? 아마 유저 메모리 풀 영역이라 그런 듯.
  vme->writable=true;
  {
    //이 vme를 프로세스의 해시테이블에 넣어준다.
    bool flag_insert_vme=vm_insert(&thread_current()->vm_hash,vme);
    if(!flag_insert_vme){
      printf("failed insert_vme");
      free(vme);
      return false;
    }
    //새로 받은 페이지를 현 프로세스의 페이지디렉토리에 넣어준다.
    bool flag_install_page=install_page(vme->vaddr,new_page,vme->writable);
    if(!flag_install_page){
      printf("failed flag_install_page");
      free(vme);
      return false;
    }
    return true;
  }
}
/*prj4*/