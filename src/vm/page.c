#include "page.h"
#include "threads/vaddr.h"
#include "threads/thread.h"

//return hash_elem's hash value
static unsigned vm_hash_func(const struct hash_elem *e, void *aux){
    //find vm_entry where placed inside the hash_elem
    struct vm_entry *vme=hash_entry(e,struct vm_entry,hash_elem);
    //return hash_elem's hash
    //hash_int->hash_bytes << return hash value
    return hash_int(vme->vaddr);
}

//return vm_entry with smaller vaddr
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux){
    struct vm_entry* va=hash_entry(a,struct vm_entry,hash_elem);
    struct vm_entry* vb=hash_entry(b,struct vm_entry,hash_elem);
    //printf("va->vaddr : %p   |   vb->vaddr : %p\n",va->vaddr,vb->vaddr);
    //true if a < b
    return (va->vaddr<vb->vaddr);
}

void vm_entry_delete_func(struct hash_elem* e,void* aux){
    //find vm_entry (vm_entry : includes hash_elem e)
    struct vm_entry *vme=hash_entry(e,struct vm_entry,hash_elem);
    if(vme){
        //if loaded -> free physical memory & free page which includes vme
        if(vme->is_loaded){
            page_delete_func(vme);
            //clear virtual memory
            pagedir_clear_page(thread_current()->pagedir,vme->vaddr);
        }
        //free vm_entry
        free(vme);
    }
}

//해시테이블을 만들어서 vm에게 할당해줌
void vm_init(struct hash *vm_hash){
    //두번째 인자인 hash 함수로 원소의 해시값을 결정함
    //세번째 인자인 less 함수는 2개의 원소(a,b)의 값을 비교하여 a가 더 작으면 true를 반환함.
    hash_init (vm_hash, vm_hash_func, vm_less_func, NULL);
}

void vm_clear(struct hash* vm){
    //printf("trying to clear...\n");
    hash_destroy(vm,vm_entry_delete_func);
}

bool vm_insert(struct hash* vm, struct hash_elem* e){
    struct hash_elem* try;
    try=hash_insert(vm,e);
    //hash was already full
    if(try==NULL) return true;
    else return false;
}

//해시테이블에서 vaddr을 가진 vm_entry를 찾아옴
struct vm_entry *search(void* vaddr){
    struct vm_entry mikki;
    struct vm_entry* real;
    struct hash_elem* hash_elem;

    //vme에는 각 페이지의 주소가 있으므로, page_addr을 갖는 vme를 찾음
    //vme를 찾으려면 list_elem -> hash_elem -> vme 순으로 찾아야하는데, hash_elem은 vme로만 찾을 수 있다.
    //vme에 저장된 vaddr로 hash table의 bucket을 정하고, bucket 내부의 list_elem에 대해 list_elem -> hash_elem -> vme를 찾아 vme의 vaddr과 처음에 넣어준 vaddr을 비교한다.
    //그래서 찾으려는 vme와 같은 vaddr을 가지는 vme를 새로 만들어서 얘를 넣어주면 진짜 vme를 찾아온다.... 진짜 신기한 자료구조다....
    mikki.vaddr=pg_round_down(vaddr);
    hash_elem=hash_find(&thread_current()->vm_hash,&mikki.hash_elem);

    //만약 찾는 vaddr에 해당하는 hahs_elem가 없다면 hash_find는 null을 리턴함.
    if(hash_elem==NULL){
        return NULL;
    }
    //찾는 vaddr에 해당하는 hash_elem이 있다면 이를 포함하는 vme를 찾아옴.
    else{
        real=hash_entry(hash_elem,struct vm_entry,hash_elem);
        return real;
    }
}