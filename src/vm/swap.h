#ifndef Swap
#define Swap

#include <stddef.h>
#define FREE 0
#define USED 1

size_t swap_out(void* kaddr);
void swap_in(size_t idx, void* kaddr);
void swap_init();

#endif Swap
