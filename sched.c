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

  frame_t * frame = parent->frame;
  SAFE_ASSERT(frame != NULL);

  if (frame->ptr == stack_shptr(frame->top, deq->tid)) {
    jtptr->frame = frame;
    frame->top = frptr->rsp;
    frame->ptr = stack_shptr(frame->top, deq->tid);

    DEBUG_PRINTV("promote (borrow): jtptr=%p frame=%p top=%p\n",
        jtptr, frame, frame->top);
  } else {
    jtptr->frame = malloc(sizeof(frame_t));
    jtptr->frame->btm = frame->top;

    frame = jtptr->frame;
    frame->top = frptr->rsp;
    frame->ptr = stack_shptr(frame->top, deq->tid);

    DEBUG_PRINTV("promote (new): jtptr=%p frame=%p top=%p btm=%p\n",
        jtptr, frame, frame->top, frame->btm);
  }


  SAFE_ASSERT(frame->top != NULL);
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

static inline void * import(const frame_t * frame)
{
  /** Copy stack prefix. */
  void * dest = frame->top;
  void * addr = frame->ptr;
  size_t size = _stack_bottom - dest;

  memcpy(dest, addr, size);
  return stack_shptr(dest, _tid);
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

  frame_t * frame = jtptr->frame;
  frame->ptr = import(frame);

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
    frame_t * frame = ((joint_t *) _exit_fr.jtp)->frame;
    memcpy(frame->top, frame->ptr, frame->btm - frame->top);
    free(frame->ptr);
    free(frame);
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
    _joint.frame->top = _exit_fr.rsp;
    _exit_fr.jtp = &_joint;

    unlock(&_done);
    fibrile_yield(&_exit_fr);
  }

AFTER_EXIT:
  barrier(_nprocs);
  return;
}

