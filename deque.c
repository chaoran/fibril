#include "tls.h"
#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "stack.h"

void fibrile_push(fibril_t * frptr)
{
  int T = DEQ.tail;

  _tls.buff[T] = frptr;
  DEQ.tail = T + 1;

  DEBUG_DUMP(3, "push:", (frptr, "%p"), (T, "%d"));
}

int fibrile_pop()
{
  int T = DEQ.tail;

  if (T == 0) return 0;

  DEQ.tail = --T;

  fence();
  int H = DEQ.head;

  if (H > T) {
    DEQ.tail = T + 1;

    lock(&DEQ.lock);
    H = DEQ.head;

    if (H > T) {
      DEQ.head = 0;
      DEQ.tail = 0;

      unlock(&DEQ.lock);
      return 0;
    }

    DEQ.tail = T;
    unlock(&DEQ.lock);
  }

  void * frptr = _tls.buff[T];

  DEBUG_DUMP(3, "pop:", (frptr, "%p"), (T, "%d"));
  DEBUG_ASSERT(frptr != NULL);
  return 1;
}

fibril_t * deque_steal(deque_t * deq)
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

  fibril_t * frptr = ((tls_t *) deq)->buff[H];
  DEBUG_ASSERT(frptr != NULL);
  DEBUG_DUMP(1, "steal:", (deq->tid, "%d"), (frptr, "%p"), (H, "%d"));

  frptr = (void *) frptr + STACK_OFFSETS[deq->tid];

  joint_t * jtptr = frptr->jtp;

  if (jtptr) {
    lock(&jtptr->lock);
    jtptr->count++;
  } else {
    jtptr = joint_create(frptr, deq);
    frptr->jtp = jtptr;
    deq->jtptr = jtptr;
  }

  unlock(&deq->lock);
  return frptr;
}

