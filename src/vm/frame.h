#ifndef __LIB_KERNEL_LIST_H
#include "../lib/kernel/list.h"
#endif


void framelist_init(struct list* frame_list);
void framelist_insert(struct list* frame_list, struct page* page);