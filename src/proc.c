#include <stdlib.h>
#include <pthread.h>
#include "sync.h"
#include "proc.h"
#include "stack.h"
#include "debug.h"
#include "deque.h"
#include "param.h"

__thread int _tid;

static __thread fibril_t * _restart;
static __thread fibril_t * _frptr;
static deque_t ** _deqs;
static fibril_t * _stop;

__attribute__((noreturn)) static
void execute(fibril_t * frptr)
{
  void * top = stack_setup(frptr);
  sync_unlock(frptr->lock);

  DEBUG_DUMP(2, "execute:", (frptr, "%p"), (top, "%p"));
  fibrili_longjmp(&frptr->state, top);
}

static void yield(fibril_t * frptr)
{
  /** Unmap partial stack if current stack has the frame. */
  if (frptr->stack.ptr == fibrili_deq.stack) {
    stack_uninstall(frptr);
  }
  /** If current stack is not the main stack, free the whole stack. */
  else if (fibrili_deq.stack != PARAM_STACK_ADDR) {
    free(fibrili_deq.stack);
  }

  sync_unlock(frptr->lock);
}

static void schedule(int id, int nprocs)
{
  fibril_t fr;

  fibril_init(&fr);
  _restart = &fr;

  if (0 == fibrili_setjmp(&fr.state)) {
    if (id == 0) return;
  } else {
    yield(_frptr);
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
  else fibrili_longjmp(&_stop->state, _stop->stack.top);
}

void proc_start(int id, int nprocs)
{
  _tid = id;

  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;
    /** Setup deque pointers. */
    _deqs = malloc(sizeof(deque_t * [nprocs]));
  }

  sync_barrier(nprocs);
  _deqs[id] = &fibrili_deq;
  sync_barrier(nprocs);

  DEBUG_DUMP(2, "proc_start:", (id, "%d"), (_deqs[id], "%p"));

  schedule(id, nprocs);
}

void proc_stop()
{
  fibril_t fr;
  _stop = &fr;

  DEBUG_DUMP(2, "proc_stop:", (_stop, "%p"), (fibrili_deq.stack, "%p"));

  if (_tid != 0) {
    fibril_init(&fr);

    if (0 == fibrili_setjmp(&fr.state)) {
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
  fibrili_longjmp(&_restart->state, _restart->stack.top);
}

