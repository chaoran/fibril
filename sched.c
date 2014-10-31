#include "tls.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "joint.h"
#include "sched.h"
#include "stack.h"
/*#include "fibril.h"*/
#include "fibrili.h"

static int _done = 1;
static fibril_t _exit_fr;

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

void sched_work(int me, int nprocs)
{
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
  free(jtptr);

  if (me) exit(0);
  else fibrile_resume(&_exit_fr, NULL, 0);
}

void sched_exit()
{
  if (_tid == 0) {
    unlock(&_done);
  } else {
    fibril_make(&_exit_fr);

    joint_t * jtp = malloc(sizeof(joint_t));
    jtp->lock = 1;
    jtp->count = 0;

    _exit_fr.jtp = jtp;

    fibrile_save(&_exit_fr, &&AFTER_EXIT);
    unlock(&_done);
    fibrile_yield(&_exit_fr);
  }

AFTER_EXIT: return;
}

