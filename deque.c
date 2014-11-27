#include <stddef.h>
#include "deque.h"
#include "atomic.h"

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

struct _fibril_t * deque_steal(deque_t * deq)
{
  atomic_lock(deq->lock);

  int head = deq->head++;

  atomic_fence();

  if (head >= deq->tail) {
    deq->head--;
    atomic_unlock(deq->lock);
    return NULL;
  }

  struct _fibril_t * frptr = deq->buff[head];

  atomic_lock(frptr->lock);

  int count = frptr->count;
  frptr->count = count < 0 ? 1 : count + 1;

  atomic_unlock(deq->lock);
  atomic_unlock(frptr->lock);

  return frptr;
}

