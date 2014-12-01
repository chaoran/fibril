#include <stdlib.h>
#include <pthread.h>
#include "sync.h"
#include "sched.h"
#include "debug.h"
#include "deque.h"
#include "param.h"

__thread int _tid;

static __thread fibril_t * _restart;
static deque_t ** _deqs;
static fibril_t * _stop;

__attribute__((noreturn, noinline)) static
void execute(fibril_t * frptr)
{
  size_t size = PARAM_STACK_SIZE;
  void * stack = malloc(size);

  frptr->stack = stack;
  sync_unlock(frptr->lock);

  DEBUG_DUMP(2, "execute:", (frptr, "%p"), (stack, "%p"));

  fibrili_longjmp(frptr, stack + size);
}

static inline
void schedule(int id, int nprocs)
{
  fibril_t fr;
  _restart = &fr;

  if (0 == fibrili_setjmp(&fr) && id == 0) {
    return;
  }

  while (sync_load(_stop) == NULL) {
    int victim = rand() % nprocs;

    if (victim != id) {
      fibril_t * frptr = deque_steal(_deqs[victim]);

      if (frptr) {
        DEBUG_DUMP(1, "steal:", (victim, "%d"), (frptr, "%p"));
        execute(frptr);
      }
    }
  }

  sync_barrier(PARAM_NUM_PROCS);

  if (id) pthread_exit(NULL);
  else fibrili_longjmp(_stop, NULL);
}

void sched_start(int id, int nprocs)
{
  _tid = id;

  /** Setup deque pointers. */
  if (id == 0) {
    _deqs = malloc(sizeof(deque_t * [nprocs]));
  }

  sync_barrier(nprocs);
  _deqs[id] = &fibrili_deq;
  sync_barrier(nprocs);

  DEBUG_DUMP(2, "sched_start:", (id, "%d"), (_deqs[id], "%p"));

  schedule(id, nprocs);
}

void sched_stop()
{
  fibril_t fr;
  _stop = &fr;

  if (_tid != 0) {
    if (0 == fibrili_setjmp(&fr)) {
      fibrili_yield(_stop);
    }
  } else {
    sync_barrier(PARAM_NUM_PROCS);
  }

  free(_deqs);
}

void fibrili_yield(fibril_t * frptr)
{
  sync_unlock(frptr->lock);
  fibrili_longjmp(_restart, NULL);
}

void fibrili_resume(fibril_t * frptr)
{
  int count;

  sync_lock(frptr->lock);
  count = frptr->count--;

  DEBUG_DUMP(3, "resume:", (frptr, "%p"), (count, "%d"));

  if (count == 0) {
    sync_unlock(frptr->lock);

    free(frptr->stack);
    fibrili_longjmp(frptr, NULL);
  } else {
    fibrili_yield(frptr);
  }
}


