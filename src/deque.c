#include <stddef.h>
#include "sync.h"
#include "debug.h"
#include "deque.h"

__thread deque_t fibrili_deq;

struct _fibril_t * deque_steal(deque_t * deq)
{
  sync_lock(deq->lock);

  int head = deq->head++;

  sync_fence();

  if (head >= deq->tail) {
    deq->head--;
    sync_unlock(deq->lock);

    return NULL;
  }

  struct _fibril_t * frptr = deq->buff[head];
  DEBUG_ASSERT(frptr != NULL);

  int count = frptr->count;

  if (count < 0) {
    frptr->count = 1;
    frptr->stack.ptr = deq->stack;
  } else {
    frptr->count = count + 1;
  }

  sync_unlock(deq->lock);
  return frptr;
}

