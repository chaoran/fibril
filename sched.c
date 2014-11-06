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

    fibril_t * frptr = deque_steal(DEQ.deqs[victim]);

    if (frptr == NULL) continue;

    joint_t * jtptr = frptr->jtp;

    joint_import(jtptr);
    unlock(&jtptr->lock);

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

