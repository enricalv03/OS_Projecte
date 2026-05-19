#include "wait_queue.h"

void wait_queue_init(wait_queue_t* q) {
  if (!q) return;
  q->head = 0;
}

int wait_queue_contains_pid(const wait_queue_t* q, unsigned int pid) {
  if (!q || pid == 0) return 0;
  for (pcb_t* p = q->head; p; p = p->next) {
    if (p->pid == pid) return 1;
  }
  return 0;
}

void wait_queue_enqueue_unique(wait_queue_t* q, pcb_t* proc) {
  if (!q || !proc || proc->pid == 0) return;
  if (wait_queue_contains_pid(q, proc->pid)) return;
  proc->prev = 0;
  proc->next = q->head;
  q->head = proc;
}

pcb_t* wait_queue_remove_pid(wait_queue_t* q, unsigned int pid) {
  if (!q || pid == 0) return 0;
  pcb_t** pp = &q->head;
  while (*pp) {
    pcb_t* p = *pp;
    if (p->pid == pid) {
      *pp = p->next;
      p->next = 0;
      p->prev = 0;
      return p;
    }
    pp = &((*pp)->next);
  }
  return 0;
}

pcb_t* wait_queue_pop(wait_queue_t* q) {
  if (!q || !q->head) return 0;
  pcb_t* p = q->head;
  q->head = p->next;
  p->next = 0;
  p->prev = 0;
  return p;
}
