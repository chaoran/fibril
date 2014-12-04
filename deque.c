#include <stddef.h>
#include "sync.h"
#include "debug.h"
#include "deque.h"

__thread deque_t fibrili_deq;

void fibrili_push(struct _fibril_t * frptr)
{
  fibrili_deq.buff[fibrili_deq.tail++] = frptr;
}

int fibrili_pop(void)
{
  int tail = fibrili_deq.tail;

  if (tail == 0) return 0;

  fibrili_deq.tail = --tail;

  fibrili_fence();

  if (fibrili_deq.head > tail) {
    fibrili_deq.tail = tail + 1;

    fibrili_lock(fibrili_deq.lock);

    if (fibrili_deq.head > tail) {
      fibrili_deq.head = 0;
      fibrili_deq.tail = 0;

      fibrili_unlock(fibrili_deq.lock);
      return 0;
    }

    fibrili_deq.tail = tail;
    fibrili_unlock(fibrili_deq.lock);
  }

  return 1;
}

frame_t deque_steal(deque_t * deq)
{
  frame_t fm;

  sync_lock(deq->lock);

  int head = deq->head++;

  sync_fence();

  if (head >= deq->tail) {
    deq->head--;
    sync_unlock(deq->lock);

    fm.frptr = NULL;
    return fm;
  }

  fm.frptr = deq->buff[head];
  fm.stack = deq->stack;

  DEBUG_ASSERT(fm.frptr != NULL && fm.stack != NULL);

  sync_lock(fm.frptr->lock);

  int count = fm.frptr->count;
  fm.frptr->count = count < 0 ? 1 : count + 1;

  sync_unlock(deq->lock);
  return fm;
}

