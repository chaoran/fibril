#include "sched.h"
#include "atomic.h"
#include "fibril.h"

typedef struct _fibrili_deque_t deque_t;

__thread deque_t fibrili_deq;

int fibrili_join(fibril_t * frptr)
{
  atomic_lock(frptr->lock);

  if (frptr->count-- == 0) {
    atomic_unlock(frptr->lock);
    return 1;
  }

  return 0;
}

void fibrili_resume(fibril_t * frptr)
{
  int count;

  atomic_lock(frptr->lock);
  count = frptr->count--;
  atomic_unlock(frptr->lock);

  if (count == 0) {
    sched_resume(frptr);
  } else {
    sched_resched();
  }
}

void fibrili_yield(fibril_t * frptr)
{
  atomic_unlock(frptr->lock);
  sched_resched();
}

