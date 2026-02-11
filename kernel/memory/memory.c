#include "memory.h"

void read_memory_map(memory_map_t* map){
  map->count = *(unsigned int*)0x1FFC;
  memory_entry_t* src = (memory_entry_t*)0x2000;
  for (unsigned int i = 0; i < map->count && i < MAX_MEMORY_ENTRIES; i++)
  {
    map->entries[i] = src[i];
  }
  
}