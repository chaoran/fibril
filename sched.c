#include "tls.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "joint.h"
#include "sched.h"
#include "stack.h"
#include "fibrili.h"

static int _done = 1;
static fibril_t _exit_fr;

static inline
joint_t * joint_new(const fibril_t * frptr, joint_t * parent,
    const deque_t * deq)
{
  SAFE_ASSERT(parent != NULL);

  joint_t * jtptr = malloc(sizeof(joint_t));
  jtptr->lock = 1;
  jtptr->count = 1;
  jtptr->parent = parent;

  if (parent->stptr->off == STACK_OFFSETS[deq->tid]) {
    jtptr->stptr = parent->stptr;
    jtptr->stptr->top = frptr->rsp;

    DEBUG_PRINTV("promote (borrow): jtptr=%p stptr=%p top=%p\n",
        jtptr, jtptr->stptr, jtptr->stptr->top);
  } else {
    jtptr->stptr = &jtptr->stack;
    jtptr->stptr->btm = parent->stptr->top;
    jtptr->stptr->top = frptr->rsp;
    jtptr->stptr->off = STACK_OFFSETS[deq->tid];

    DEBUG_PRINTV("promote (new): jtptr=%p stptr=%p top=%p btm=%p\n",
        jtptr, jtptr->stptr, jtptr->stptr->top, jtptr->stptr->btm);
  }

  return jtptr;
}

static inline joint_t * promote(fibril_t * frptr, deque_t * deq)
{
  joint_t * jtptr;

  /** If the frame has already been stolen, use that joint pointer. */
  if (NULL != (jtptr = frptr->jtp)) {
    lock(&jtptr->lock);
    jtptr->count++;
  }

  /** Otherwise create a new joint pointer. */
  else {
    jtptr = joint_new(frptr, deq->jtptr, deq);
    frptr->jtp = jtptr;
    deq->jtptr = jtptr;
  }

  return jtptr;
}

static inline void * steal(deque_t * deq)
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
  DEBUG_PRINTC("steal: victim=%d frptr=%p\n", deq->tid, frptr);

  joint_t * jtptr = promote(stack_shptr(frptr, deq->tid), deq);
  unlock(&deq->lock);

  joint_import(jtptr);

  jtptr->stptr->off = STACK_OFFSETS[_tid];
  _deq.jtptr = jtptr;

  unlock(&jtptr->lock);

  return frptr;
}

void sched_work(int me, int nprocs)
{
  /** Allocate a joint before stealing. */
  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = steal(_deq.deqs[victim]);

    if (frptr == NULL) continue;

    sched_resume(frptr);
  }

  unlock(&_done);

  if (me) {
    barrier(_nprocs);
    exit(0);
  } else {
    joint_t * jtptr = _exit_fr.jtp;
    joint_import(jtptr);
    free(jtptr->stptr->top + jtptr->stptr->off);
    sched_resume(&_exit_fr);
  }
}

void sched_exit()
{
  if (_tid == 0) {
    unlock(&_done);
  } else {
    fibril_make(&_exit_fr);

    fibrile_save(&_exit_fr, &&AFTER_EXIT);
    _joint.stptr->top = _exit_fr.rsp;
    _exit_fr.jtp = &_joint;

    unlock(&_done);
    fibrile_yield(&_exit_fr);
  }

AFTER_EXIT:
  barrier(_nprocs);
  return;
}

