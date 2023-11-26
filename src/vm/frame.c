#include "frame.h"

void framelist_init(struct list* frame_list){
    list_init(frame_list);
}

//insert page at the end of the list
void framelist_insert(struct list* frame_list, struct page* page){
    list_push_back(frame_list,page->felem);
}

void* page_replace(){
    //1. 
}