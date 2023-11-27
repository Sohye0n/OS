#ifndef Page
#define Page
#ifndef __LIB_KERNEL_HASH_H
#include <hash.h>
#endif

#define BIN 0
#define FILE 1
#define ANONYMOUS 2

/*page나 vm_entry나 비슷한 역할인데 왜 2개를 따로 정의하나? 
>> page replacement를 구현하기 위해.
vm_entry는 프로세스가 사용하는 모든 메모리가 갖고 있음.
page는 이 중 물리 메모리(stack)에 load된 vme만을 가리킴. 
*/
struct page{
    //physical address
    void* physical_addr;
    //vm_entry
    struct vm_entry* vme;
    //list elem
    struct list_elem pelem;
    //to get page directory
    struct thread* thr;
};

struct vm_entry{
    uint8_t type;
    void* vaddr;
    bool writable;
    bool is_loaded;
    struct file* file;
    struct list_elem mmap_elem;
    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;
    size_t swap_slot;
    struct hash_elem hash_elem;
};

void vm_init(struct hash *vm);
bool vm_insert(struct hash* vm, struct hash_elem* e);
//return vm_entry with such vaddr
struct vm_entry *search(void* vaddr);
#endif