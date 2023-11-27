#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "vm/page.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */ 
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

//   /*내가 추가함... 테케 bad-sp 땜에. 커널이 유저 영역을 참조하려고 하면 막아야 함. 근데 커널이 왜 참조하지....*/
//     if (!user) exit(-1);
//     if (is_kernel_vaddr(fault_addr)) exit(-1);
//     if (not_present) {
//     //printf("panic\n");
//     exit(-1);
//     }

   /* To implement virtual memory, delete the rest of the function
      body, and replace it with code that brings in the page to
      which fault_addr refers. */
   printf ("Page fault at %p: %s error %s page in %s context.\n",
            fault_addr,
            not_present ? "not present" : "rights violation",
            write ? "writing" : "reading",
            user ? "user" : "kernel");

   //페이지폴트에 대한 처리를 해줘야 함
   //말 그대로 물리메모리에 올라와있지 않은 메모리를 참조했을 때
   if(not_present){
      //해시테이블을 뒤져서 해당 vaddr을 가진 vm_entry를 가져와서 이걸 물리메모리에 올려야함.

      //1. fault가 발생한 vaddr을 가진 vm_entry를 가져온다
      struct vm_entry* vme=search(fault_addr);
      printf("vme's type : %d\n",vme->type);
      //vme가 존재한다 -> 물리메모리에 load해주면 됨
      if(vme){
         //2. 얘를 물리 메모리에 올려야하므로 is_load 변수를 true로 해준다.
         //printf("have to load : %p\n",vme->vaddr);
         vme->is_loaded=true;
         bool success=load_to_pmemory(fault_addr,vme);
         if(!success){
            printf("failed to load\n");
            exit(-1);
         }
      }
      //vme가 NULL이다 -> 공간이 모자라다. 따라서 스택을 더 키워준다.
      else{

         //근데 커널이 유저 영역을 침범한다면?
         if(!user) exit(-1);

         //esp가 커널 영역을 침범한다면?
         if(is_kernel_vaddr(f->esp)) exit(-1);

         //스택 밖의 영역에 접근한다면 (esp보다 더 작은 주소에 접근한다면)
         if(fault_addr<f->esp) exit(-1);

         //요청 영역이 커널 영역이라면?
         //vaddr 자체는 유저 영역이어도 이걸 포함하는 페이지는 커널 영역일 수도 있음.
         void* page_where_faddr_in;
         page_where_faddr_in=pg_round_down(fault_addr);
         if(is_kernel_vaddr(page_where_faddr_in) || is_kernel_vaddr(fault_addr)) exit(-1);
         if(is_user_vaddr(fault_addr)) printf("%p user\n",fault_addr);

         //나머지 경우는 유저가 유저 영역을 사용하는 것이므로 스택을 더 확장해주면 된다.
         printf("calling stack_grow function\n");
         bool stack_grow=stack_growth(fault_addr);
         //만약 스택 확장에 실패했다면
         if(!stack_grow) exit(-1);
      }
   }
   else{
      printf("eke\n");
      exit(-1);
   }

}

