#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include "process.h"

typedef struct {
  pcb_t* head;
} wait_queue_t;

void wait_queue_init(wait_queue_t* q);
int wait_queue_contains_pid(const wait_queue_t* q, unsigned int pid);
void wait_queue_enqueue_unique(wait_queue_t* q, pcb_t* proc);
pcb_t* wait_queue_remove_pid(wait_queue_t* q, unsigned int pid);
pcb_t* wait_queue_pop(wait_queue_t* q);

#endif
