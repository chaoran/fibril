#include "tls.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "joint.h"
#include "sched.h"
#include "stack.h"
#include "fibrili.h"

static int _done;

static inline void * steal(deque_t * deq, joint_t * jtptr)
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

  fibril_t * rfrptr = stack_shptr(frptr, deq->tid);
  rfrptr->jtp = jtptr;

  unlock(&deq->lock);
  return frptr;
}

int sched_work()
{
  /** Setup frame if this is the first time. */
  if (_deq.base == NULL) {
    _deq.base = this_frame();

    if (_tid == 0) lock(&_done);

    barrier(&_barrier, _nprocs);
    DEBUG_PRINTI("sched_work: base=%p\n", _deq.base);

    if (_tid == 0) return SCHED_DONE;
  }

  /** Main loop. */
  int nprocs = _nprocs;
  int me = _tid;

  /** Allocate a joint before stealing. */
  joint_t * jtptr = malloc(sizeof(joint_t));
  jtptr->lock = 0;
  jtptr->count = 1;

  lock(&jtptr->lock);

  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = steal(_deq.deqs[victim], jtptr);

    if (frptr == NULL) continue;

    void * stack = ((fibril_t *) stack_shptr(frptr, victim))->rsp;
    stack_import(stack, stack_shptr(stack, victim));
    jtptr->stack = stack_shptr(stack, me);
    unlock(&jtptr->lock);

    DEBUG_PRINTC("steal: victim=%d frptr=%p jtptr=%p\n", victim, frptr, jtptr);
    sched_resume(frptr);
  }

  unlock(&_done);
  return SCHED_DONE;
}

void sched_exit()
{
  unlock(&_done);
}

