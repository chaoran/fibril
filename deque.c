#include "util.h"
#include "page.h"
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

fibril_t * deque_steal(deque_t * deq, int vic, joint_t * jtp)
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

  intptr_t offset = STACK_OFFSETS[vic];
  fibril_t * rfrptr = (void *) frptr + offset;

  joint_t * jtptr = rfrptr->jtp;

  if (jtptr) {
    lock(&jtptr->lock);
    jtptr->count++;
  } else {
    jtptr = jtp;

    /** Initialize jtptr. */
    jtp = deq->jtptr;

    if (jtp->stptr->off == offset) {
      jtptr->stptr = jtp->stptr;
    } else {
      jtptr->stack.btm = jtp->stptr->top;
    }

    jtptr->stptr->top = rfrptr->rsp;
    jtptr->parent = jtp;

    deq->jtptr = jtptr;
    rfrptr->jtp = jtptr;
  }

  jtptr->stptr->off = STACK_OFFSETS[TID];
  unlock(&deq->lock);

  DEBUG_DUMP(1, "deque_steal:", (vic, "%d"), (frptr, "%p"), (jtptr, "%p"),
      (jtptr->stptr, "%p"), (jtptr->stptr->top, "%p"),
      (jtptr->stptr->btm, "%p"));
  return rfrptr;
}

