#include "page.h"

//return hash_elem's hash value
hash_hash_func vm_hash_func(const struct hash_elem *e, void *aux){
    //find vm_entry where placed inside the hash_elem
    struct vm_entry *vme=hash_entry(e,struct vm_entry,elem);
    //return hash_elem's hash
    //hash_int->hash_bytes << return hash value
    return hash_int(vme->vaddr);
}

//return vm_entry with smaller vaddr
hash_less_func vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux){
    struct vm_entry* va=hash_entry(a,struct vm_entry,elem);
    struct vm_entry* vb=hash_entry(b,struct vm_entry,elem);
    //true if a < b
    return (va->vaddr<vb->vaddr);
}

//해시테이블을 만들어서 vm에게 할당해줌
void vm_init(struct hash *vm){
    //두번째 인자인 hash 함수로 원소의 해시값을 결정함
    //세번째 인자인 less 함수는 2개의 원소(a,b)의 값을 비교하여 a가 더 작으면 true를 반환함.
    hash_init (struct hash *vm, vm_hash_func, vm_less_func, NULL);
}
