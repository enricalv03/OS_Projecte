#include "heap.h"
#include "vmm.h"
#include "pmm.h"

#define HEAP_START_VIRT     0x10000000
#define HEAP_INITIAL_SIZE   0x100000
#define HEAP_MAX_SIZE       0x10000000
#define MIN_BLOCK_SIZE      16

typedef struct block_header
{
  unsigned int size;
  unsigned int free;
  struct block_header* next;
} block_header_t;

static block_header_t* heap_start = 0;
static unsigned int heap_size = 0;
static unsigned int heap_allocated = 0;

void heap_init(void) {
  unsigned int pages_needed = (HEAP_INITIAL_SIZE + 4095) / 4096;

  for (unsigned int i = 0; i < pages_needed; i++)
  {
    unsigned int virt_addr = HEAP_START_VIRT + (i * 4096);
    unsigned int phys_addr = vmm_alloc_and_map(virt_addr, VMM_KERNEL_RW);

    if (phys_addr == 0)
    {
      return;
    }
  }

  heap_start = (block_header_t*)HEAP_START_VIRT;
  heap_start->size = HEAP_INITIAL_SIZE;
  heap_start->free = 1;
  heap_start->next = 0;

  heap_size = HEAP_INITIAL_SIZE;
  heap_allocated = 0;
}

static block_header_t* find_free_block(unsigned int size) {
  block_header_t* current = heap_start;

  while (current)
  {
    if (current->free && current->size >= size)
    {
      return current;
    }
    current = current->next;  /* FIXED: advance iterator to avoid infinite loop */
  }
  return 0;
}

static void split_block(block_header_t* block, unsigned int size) {
  if (block->size < MIN_BLOCK_SIZE + sizeof(block_header_t))
  {
    return;
  }

  block_header_t* new_block = (block_header_t*)((char*)block + size);
  new_block->size = block->size - size;
  new_block->free = 1;
  new_block->next = block->next;

  block->size = size;
  block->next = new_block;
  
}

void* malloc(unsigned int size) {
  if (size == 0)
  {
    return 0;
  }
  size = ((size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE) * MIN_BLOCK_SIZE;
  size += sizeof(block_header_t);

  block_header_t* block = find_free_block(size);

  if (!block)
  {
    return 0;
  }

  block->free = 0;
  split_block(block, size);

  heap_allocated += block->size;

  return (void*)((char*)block + sizeof(block_header_t));
}

static void merge_free_blocks(void) {
  block_header_t* current = heap_start;

  while (current && current->next)
  {
    if (current->free && current->next->free)
    {
      current->size += current->next->size;
      current->next = current->next->next;
    } else {
      current = current->next;
    }
  }
}

void free(void* ptr) {
  if (!ptr)
  {
    return;
  }
  
  block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));

  if (block->free)
  {
    return;
  }

  block->free = 1;

  heap_allocated -= block->size;

  merge_free_blocks();
}

unsigned int heap_get_allocated(void) {
  return heap_allocated;
}

unsigned int heap_get_free(void) {
  return heap_size - heap_allocated;
}
