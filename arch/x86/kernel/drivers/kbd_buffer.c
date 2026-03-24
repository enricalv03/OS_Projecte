#include "kbd_buffer.h"

#define KBD_BUF_SIZE 128

static volatile unsigned char buf[KBD_BUF_SIZE];
static volatile unsigned int head = 0;
static volatile unsigned int tail = 0;

void kbd_buffer_put(char c){
  unsigned int next = (head + 1) % KBD_BUF_SIZE;
  if(next == tail){
    return;
  }
  buf[head] = (unsigned char)c;
  head=next;
}

int kbd_buffer_get(void){
  if (head == tail) return -1;
  unsigned char c = buf[tail];
  tail = (tail+1) % KBD_BUF_SIZE;
  return (int)c;
}