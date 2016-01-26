#include "fifo.h"
#include "sync.h"

#define POOL_SIZE 512
static void * volatile _pool[POOL_SIZE];

void enqueue(void * stack)
{
  static long volatile _tail __attribute__((aligned(128)));
  long i = sync_fadd(_tail, 1) % POOL_SIZE;

  _pool[i] = stack;
}

void * dequeue()
{
  static long volatile _head __attribute__((aligned(128)));
  long i = sync_fadd(_head, 1) % POOL_SIZE;

  void * stack = _pool[i];
  while (!stack) stack = _pool[i];

  _pool[i] = (void *) 0;
  return stack;
}
