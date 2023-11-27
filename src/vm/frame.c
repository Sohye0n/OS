#include "frame.h"
#include "threads/thread.h"
#define PAL_USER 004

//initialize framelist
void framelist_init(void){
    //init frame_list
    list_init(&frame_list);
    //init frame_pointer
    cur=NULL;
    //init lock
    lock_init(&frame_lock);
}

//insert page at the end of the list
void framelist_insert(struct list* frame_list, struct page* page){
    list_push_back(frame_list,&page->pelem);
    //printf("insert : %p\n",list_entry(list_end(frame_list)->prev,struct page,pelem)->vme->vaddr);
}

//delete page at the framelist
void framelist_delete(struct list* frame_list, struct page* page){
    //printf("delete\n");
    // struct list_elem* start_elem=list_front(frame_list);
    // struct list_elem* end_elem=list_end(frame_list);
    // struct list_elem* cur_elem=start_elem;
    // struct page* cur_page;
    // int cnt=0;

    // while(1){
    //     //current struct page
    //     cur_page=list_entry(cur_elem, struct page, pelem);

    //     //find
    //     if(cur_page==page){
    //         printf("found at %d, : %p\n",cnt,cur_page->vme->vaddr);
    //         //1 element
    //         if(start_elem==end_elem) return;

    //         //more than 2 element
    //         else{
    //             if(cur_elem==start_elem){
    //                 printf("start==cur\n");
    //                 end_elem->next=cur_elem->next;
    //                 cur_elem->next->prev=end_elem;
    //             }
    //             else{
    //                 cur_elem->prev->next=cur_elem->next;
    //                 cur_elem->next->prev=cur_elem->prev;
    //             }
    //             return;
    //         }
    //     }

    //     //not found
    //     else{
    //         cur_elem=list_next(cur_elem);
    //         cnt++;
    //         if(cur_elem==list_end(frame_list)){
    //             return;
    //         }
    //     }
    // }
    list_remove(&page->pelem);
}

//search framelist
void page_replace(struct list* frame_list){
    //printf("page_replace\n");
    //printf("front : %p\n",list_entry(list_front(frame_list),struct page, pelem)->vme->vaddr);
    //printf("end : %p\n",list_entry(list_end(frame_list)->prev,struct page, pelem)->vme->vaddr);
    //1.find replacable page ()
    //start from saved spot
    //struct list_elem* cur=frame_pointer;
    struct page* pg;
    bool is_accessed;
    void* newpage;

    //if frame list is empty
    if(cur==NULL){
        cur=list_begin(frame_list);
        if(list_empty(cur)){
            //printf("frame list is empty\n");
            //return NULL?
            return NULL;
        }
    }

    //else
        while(1){
            printf("cur : %p || accessed : %d(thread %d)\n",list_entry(cur,struct page, pelem)->vme->vaddr,
            pagedir_is_accessed(list_entry(cur,struct page, pelem)->thr->pagedir,
            list_entry(cur,struct page, pelem)->vme->vaddr),
            thread_current()->tid
            );
            //get certain page
            pg=list_entry(cur,struct page,pelem);
            //printf("current page's vaddr : %p\n",pg->vme->vaddr);
            //check if accessed
            is_accessed=pagedir_is_accessed(pg->thr->pagedir,pg->vme->vaddr);
            
            //if not accessed -> change to accessed & replace
            if(!is_accessed){
                printf("current page's vaddr : %p\n",pg->vme->vaddr);
                //change to accessed
                pagedir_set_accessed(pg->thr->pagedir,pg->vme->vaddr,true); 
                //replace
                //1. free current page
                pagedir_clear_page (pg->thr->pagedir, pg_round_down(pg->vme->vaddr));
                palloc_free_page(pg->physical_addr);
                //2. remove current page from the frame_list
                framelist_delete(frame_list,pg);
                //3. set cur to next page
                
                cur=list_next(cur);
                if(cur==list_end(frame_list)){
                    if(list_empty(frame_list)) cur=NULL;
                    else cur=list_front(frame_list);
                }
                //3. return
                //printf("next : %p || %d(thread : %d)\n",list_entry(cur,struct page,pelem)->vme->vaddr,
                //pagedir_is_accessed(pg->thr->pagedir,pg->vme->vaddr), thread_current()->tid);
                return newpage;
            }

            //if accessed -> search next. search until not accessed page found
            else{
                //set current page to not accessed
                pagedir_set_accessed(pg->thr->pagedir,pg->vme->vaddr,false);
                //if(pg->vme->vaddr==0x804a000) printf("%d\n",pagedir_is_accessed(pg->thr->pagedir,pg->vme->vaddr));
                //search next
                //if cur is last element in framelist -> set cur to first
                cur=list_next(cur);
                if(cur==list_end(frame_list)){
                    if(list_empty(frame_list)) cur=NULL;
                    else cur=list_front(frame_list);
                }
                
            }
        }
}