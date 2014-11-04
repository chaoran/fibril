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

static inline joint_t * promote(fibril_t * frptr, deque_t * deq)
{
  joint_t * jtptr;

  /** If the frame has already been stolen, use that joint pointer. */
  if (NULL != (jtptr = frptr->jtp)) {
    lock(&jtptr->lock);
    jtptr->count++;

    DEBUG_PRINTV("promote (reuse): jtptr=%p count=%d\n", jtptr, jtptr->count);
  } else {
    jtptr = deq->jtptr;

    /** If the deque's joint pointer is not stolen, use that pointer. */
    if (jtptr->stack.ptr == stack_shptr(jtptr->stack.top, deq->tid)) {
      lock(&jtptr->lock);
      jtptr->count++;

      jtptr->stack.top = frptr->rsp;
      jtptr->stack.ptr = stack_shptr(jtptr->stack.top, deq->tid);
      frptr->jtp = jtptr;

      DEBUG_PRINTV("promote (borrow): jtptr=%p count=%d top=%p\n",
          jtptr, jtptr->count, jtptr->stack.top);
    }

    /** Otherwise create a new joint pointer. */
    else {
      jtptr = malloc(sizeof(joint_t));
      jtptr->lock = 1;
      jtptr->count = 1;

      joint_t * parent = deq->jtptr;
      jtptr->parent = parent;
      jtptr->stack.btm = parent->stack.top;

      deq->jtptr = jtptr;

      jtptr->stack.top = frptr->rsp;
      jtptr->stack.ptr = stack_shptr(jtptr->stack.top, deq->tid);
      frptr->jtp = jtptr;

      DEBUG_PRINTV("promote (new): jtptr=%p count=%d top=%p btm=%p\n",
          jtptr, jtptr->count, jtptr->stack.top, jtptr->stack.btm);
    }
  }

  return jtptr;
}

static inline void import(joint_t * jtptr)
{
  /** Copy stack prefix. */
  void * dest = jtptr->stack.top;
  void * addr = jtptr->stack.ptr;
  size_t size = jtptr->stack.btm - dest;

  memcpy(dest, addr, size);

  /** Move the joint pointer. */
  jtptr->stack.ptr = stack_shptr(dest, _tid);
  _deq.jtptr = jtptr;

  unlock(&jtptr->lock);
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
  DEBUG_PRINTC("steal: victim=%d head=%d frptr=%p\n", deq->tid, H, frptr);
  SAFE_ASSERT(frptr != NULL);

  joint_t * jtptr = promote(stack_shptr(frptr, deq->tid), deq);

  unlock(&deq->lock);

  import(jtptr);
  SAFE_ASSERT(frptr->jtp == jtptr);

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

  if (me) exit(0);
  else fibrile_resume(&_exit_fr, NULL, 0);
}

void sched_exit()
{
  if (_tid == 0) {
    unlock(&_done);
  } else {
    fibril_make(&_exit_fr);

    _exit_fr.jtp = &_joint;
    _joint.stack.top = _exit_fr.rsp;

    fibrile_save(&_exit_fr, &&AFTER_EXIT);
    unlock(&_done);
    fibrile_yield(&_exit_fr);
  }

AFTER_EXIT: return;
}

