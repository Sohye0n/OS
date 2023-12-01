#include "swap.h"
#include "threads/synch.h"
#include "lib/kernel/bitmap.h"
#include "threads/vaddr.h"
#include "devices/block.h"

struct block* swapspace;
struct bitmap* swapmap;
struct lock swaplock;

//블록 : 2^9(512 byte)
//페이지 : 2^12(4kb)
//따라서 한 페이지를 디스크에 적으려면 8개의 블록 사용해야 함.

void swap_init(){
  //1. get block (swap space)
  swapspace=block_get_role(BLOCK_SWAP);
  //2. make this space to bitmap
  /*div by 8. 한 페이지당 8개의 블록을 사용할 수 있음 -> swapspace / 8 = 저장할 수 있는 페이지의 수 */
  swapmap=bitmap_create(block_size(swapspace)/8);
  //3. init bitmap
  //no page have swapped out yet -> set all to 0
  bitmap_set_all(swapmap,FREE);
  //4. init lock
  lock_init(&swaplock);
}

//content stored in address kaddr  ---write--->  swap space
//return bitmap index
size_t swap_out(void* kaddr){
  lock_acquire(&swaplock);
  //1. find free space in bitmap
  size_t ith=bitmap_scan_and_flip(swapmap,0,1,FREE);
  
  
  //2. find start point.
  //2-1. check if valid
  if(ith==BITMAP_ERROR) return -1;
  /*i번째 비트 -> i번째로 할당된 페이지. 한 페이지는 8개 블록을 쓰니까 시작점은 i*8
  0부터 시작하니까 1 빼지 말고 바로 i*8*/
  block_sector_t sector=ith*8;

  //3. write content (don't need lock)
  //for 8 block
  for(int i=0; i<8; i++){
    //printf("---------------before----------------\n");
    //hex_dump((uint32_t*)swapspace+sector+i*128,(uint32_t*)swapspace+sector+i*128,512,1);
    //block_write(블록덩어리 주소, write at, write from)
    block_write(swapspace,ith*8+i,(uint8_t*)kaddr+i*512);
    //hex_dump((uint32_t*)swapspace+sector+i,(uint32_t*)swapspace+sector+i,100,1);
    //printf("--------------after----------------\n");
    //hex_dump((uint32_t*)swapspace+sector+i*128,(uint32_t*)swapspace+sector+i*128,512,1);
  }

  //test
  //printf("swap_out to %d\n",ith);
  lock_release(&swaplock);

  //4. return bitmap index
  return ith;
}

//content stored in ith block in swap_space ---read---> address kaddr
void swap_in(size_t ith, void* kaddr){
  lock_acquire(&swaplock);
  //1. set start point
  block_sector_t sector=ith*8;

  //2. read content (don't need lock)
  //for 8 block
  if(bitmap_test(swapmap,ith)){
    for(int i=0; i<8; i++){
      //block_read(블록덩어리 주소, read at, read from)
      block_read(swapspace,ith*8+i,(uint8_t*)kaddr+i*512);
      //printf("success\n");
    }

    //3. set ith bitmap element Free
    //printf("[%d]\n",bitmap_test(swapmap,ith));
    bitmap_flip(swapmap,ith);
    //printf("[%d]\n",bitmap_test(swapmap,ith));
    //test
    //printf("%zuth swap_in\n",ith);
  }
  lock_release(&swaplock);
}
