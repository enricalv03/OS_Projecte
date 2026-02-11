#ifndef HEAP_H
#define HEAP_H

void heap_init(void);

void* malloc(unsigned int size);

void free(void* ptr);

unsigned int heap_get_allocated(void);

unsigned int heap_get_free(void);

#endif