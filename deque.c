#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "stack.h"
#include "tlmap.h"

deque_t __fibril_local__ fibrile_deq;

void fibrile_push(fibril_t * frptr)
{
  int T = fibrile_deq.tail;

  fibrile_deq.buff[T] = frptr;
  fibrile_deq.tail = T + 1;

  DEBUG_DUMP(3, "push:", (frptr, "%p"), (T, "%d"));
}

int fibrile_pop()
{
  int T = fibrile_deq.tail;

  if (T == 0) return 0;

  fibrile_deq.tail = --T;

  fence();
  int H = fibrile_deq.head;

  if (H > T) {
    fibrile_deq.tail = T + 1;

    lock(&fibrile_deq.lock);
    H = fibrile_deq.head;

    if (H > T) {
      fibrile_deq.head = 0;
      fibrile_deq.tail = 0;

      unlock(&fibrile_deq.lock);
      return 0;
    }

    fibrile_deq.tail = T;
    unlock(&fibrile_deq.lock);
  }

  void * frptr = fibrile_deq.buff[T];

  DEBUG_DUMP(3, "pop:", (frptr, "%p"), (T, "%d"));
  DEBUG_ASSERT(frptr != NULL);
  return 1;
}

fibril_t * deque_steal(deque_t * deq, int tid)
{
  lock(&deq->lock);

  int H = deq->head++;
  fence();
  int T = deq->tail;

  if (H >= T) {
    deq->head--;

    unlock(&deq->lock);
    return NULL;
  }

  fibril_t * frptr = deq->buff[H];
  DEBUG_ASSERT(frptr != NULL);
  DEBUG_DUMP(1, "steal:", (tid, "%d"), (frptr, "%p"), (H, "%d"));

  frptr = (void *) frptr + STACK_OFFSETS[tid];

  joint_t * jtptr = frptr->jtp;

  if (jtptr) {
    lock(&jtptr->lock);
    jtptr->count++;
  } else {
    jtptr = joint_create(frptr, deq, tid);
    frptr->jtp = jtptr;
    deq->jtptr = jtptr;
  }

  unlock(&deq->lock);
  return frptr;
}

