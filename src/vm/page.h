#ifndef __LIB_KERNEL_HASH_H
#include <hash.h>
#endif

#define BIN 0
#define FILE 1
#define ANONYMOUS 2

struct page{
    //physical address
    void* phsyical_addr;
    //vm_entry
    struct vm_entry* vme;
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