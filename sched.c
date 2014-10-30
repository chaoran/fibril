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

  fibril_t * rfrptr = adjust(frptr, stack_offset(deq->tid));
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
  jtptr->offset = stack_offset(me);

  lock(&jtptr->lock);

  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = steal(_deq.deqs[victim], jtptr);

    if (frptr == NULL) continue;

    intptr_t offset = stack_offset(victim);
    void * stack = ((fibril_t *) adjust(frptr, offset))->rsp;
    stack_import(stack, adjust(stack, offset));

    DEBUG_PRINTC("steal: victim=%d frptr=%p jtptr=%p stack=%p offset=%lx\n",
        victim, frptr, jtptr, stack, jtptr->offset);

    unlock(&jtptr->lock);
    sched_resume(frptr);
  }

  unlock(&_done);
  return SCHED_DONE;
}

void sched_exit()
{
  unlock(&_done);
}

