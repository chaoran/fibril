#include "tls.h"
#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "sched.h"
#include "fibrili.h"

static int _done = 1;
static fibril_t _exit_fr;

void sched_work(int me, int nprocs)
{
  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = deque_steal(fibrile_deq.deqs[victim], victim);

    if (frptr == NULL) continue;

    joint_t * jtptr = frptr->jtp;

    joint_import(jtptr);
    fibrile_deq.jtptr = jtptr;
    jtptr->stptr->off = STACK_OFFSETS[TID];

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

