#ifndef MEMORY_H
#define MEMORY_H

#define MAX_MEMORY_ENTRIES 32

typedef struct
{
  unsigned int base_low;
  unsigned int base_high;
  unsigned int length_low;
  unsigned int length_high;
  unsigned int type;
  unsigned int acpi_attrs;
} memory_entry_t;

typedef struct{
  unsigned int count;
  memory_entry_t entries[MAX_MEMORY_ENTRIES];
} memory_map_t;

void read_memory_map(memory_map_t* map);

#endif