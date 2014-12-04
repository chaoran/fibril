#include <stdlib.h>
#include <pthread.h>
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "deque.h"
#include "param.h"
#include "sched.h"
#include "stack.h"

__thread int _tid;

static __thread fibril_t * _restart;
static __thread fibril_t * volatile _frptr;
static deque_t ** _deqs;
static fibril_t * _stop;

__attribute__((noreturn)) static
void execute(frame_t fm)
{
  void * rsp = stack_setup(fm.frptr, fm.stack);
  sync_unlock(fm.frptr->lock);

  DEBUG_DUMP(2, "execute:", (fm.frptr, "%p"), (rsp, "%p"));
  fibrili_longjmp(fm.frptr, rsp);
}

static inline
void schedule(int id, int nprocs)
{
  fibril_t fr;
  _restart = &fr;

  if (0 == fibrili_setjmp(&fr)) {
    if (id == 0) return;
  } else {
    stack_uninstall(_frptr);
  }

  while (sync_load(_stop) == NULL) {
    int victim = rand() % nprocs;
    if (victim == id) continue;

    frame_t fm = deque_steal(_deqs[victim]);
    if (fm.frptr == NULL) continue;

    DEBUG_DUMP(1, "steal:", (victim, "%d"), (fm.frptr, "%p"), (fm.stack, "%p"));
    execute(fm);
  }

  sync_barrier(PARAM_NUM_PROCS);

  if (id) pthread_exit(NULL);
  else fibrili_longjmp(_stop, NULL);
}

void sched_start(int id, int nprocs)
{
  _tid = id;

  if (id == 0) {
    /** Setup stack. */
    stack_init();

    /** Setup deque pointers. */
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
  _frptr = frptr;
  sync_unlock(frptr->lock);
  fibrili_longjmp(_restart, NULL);
}

void fibrili_resume(fibril_t * frptr)
{
  int count;

  sync_lock(frptr->lock);
  count = frptr->count--;

  if (count == 0) {
    stack_reinstall(frptr);
    sync_unlock(frptr->lock);
    fibrili_longjmp(frptr, NULL);
  } else {
    fibrili_yield(frptr);
  }
}

