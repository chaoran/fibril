#include "tls.h"
#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "stack.h"

void fibrile_push(fibril_t * frptr)
{
  DEBUG_PRINTV("push: frptr=%p tail=%d\n", frptr, DEQ.tail);
  _tls.buff[DEQ.tail++] = frptr;
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

  DEBUG_PRINTV("poped: frptr=%p tail=%d\n", _tls.buff[T], T);
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
  SAFE_ASSERT(frptr != NULL);
  DEBUG_PRINTV("steal: victim=%d frptr=%p head=%d\n", deq->tid, frptr, H);

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

