#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "sched.h"
#include "shmap.h"
#include "tlmap.h"

static deque_t ** _deqs;
static int __fibril_shared__ _done;
static fibril_t __fibril_shared__ _exit_fr;

joint_t __fibril_shared__ _joint;

void sched_init(int nprocs)
{
  /** Setup deque pointers. */
  _deqs = malloc(sizeof(deque_t * [nprocs]));
  SAFE_ASSERT(_deqs != NULL);

  int i;
  for (i = 0; i < nprocs; ++i) {
    _deqs[i] = (void *) &fibrile_deq + TLS_OFFSETS[i];
  }

  /** Setup initial joint. */
  _joint.stack.top = STACK_BOTTOM;
  _joint.stack.btm = STACK_BOTTOM;
  _joint.stack.off = STACK_OFFSETS[0];
  _joint.stptr = &_joint.stack;

  fibrile_deq.jtptr = &_joint;

  lock(&_done);
}

void sched_work(int me, int nprocs)
{
  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = deque_steal(_deqs[victim], victim);

    if (frptr == NULL) continue;

    joint_t * jtptr = frptr->jtp;

    void * dest = jtptr->stptr->top;
    void * addr = dest + STACK_OFFSETS[victim];
    size_t size = STACK_BOTTOM - dest;

    memcpy(dest, addr, size);

    fibrile_deq.jtptr = jtptr;
    unlock(&jtptr->lock);
    sched_resume(frptr);
  }

  unlock(&_done);

  if (me) {
    barrier();
    exit(0);
  } else {
    joint_t * jtptr = _exit_fr.jtp;

    lock(&jtptr->lock);
    joint_import(jtptr);
    free(jtptr->stptr->top + jtptr->stptr->off);
    unlock(&jtptr->lock);

    sched_resume(&_exit_fr);
  }
}

void sched_exit()
{
  if (TID == 0) {
    unlock(&_done);
  } else {
    fibril_make(&_exit_fr);

    fibrile_save(&_exit_fr, &&AFTER_EXIT);
    _joint.lock = 1;
    _joint.stptr->top = _exit_fr.rsp;
    _exit_fr.jtp = &_joint;

    unlock(&_done);
    fibrile_yield(&_exit_fr);
  }

AFTER_EXIT:
  barrier();
  return;
}

