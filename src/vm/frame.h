#ifndef Frame
#define Frame
#ifndef __LIB_KERNEL_LIST_H
#include "../lib/kernel/list.h"
#endif
#include "page.h"
#include "swap.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

void framelist_init(void);
void framelist_insert(struct list* frame_list, struct frame* frame);
void framelist_delete(struct list* frame_list, struct frame* frame);
void page_replace(struct list* frame_list);
void page_delete_func(struct page* pg);
struct list_elem* cur;
#endif Frame