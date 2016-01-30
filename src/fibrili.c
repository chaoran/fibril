#include <stdlib.h>
#include <pthread.h>
#include "pool.h"
#include "sync.h"
#include "stack.h"
#include "debug.h"
#include "deque.h"
#include "param.h"
#include "fibrile.h"

static __thread fibril_t * _restart;
static __thread fibril_t * _frptr;
static deque_t ** _deqs;
static fibril_t * volatile _stop;

__attribute__((noreturn)) static
void longjmp(fibril_t * frptr, void * rsp)
{
  DEBUG_DUMP(3, "jump:", (frptr->pc, "%p"), (rsp, "%p"));
  sync_unlock(frptr->lock);
  __asm__ ( "mov\t%1,%%rsp\n\t"
            "mov\t%0,%%rbp\n\t"
            "jmp\t*%2\n\t"
            : : "r" (frptr->stack.btm), "r" (rsp), "r" (frptr->pc) : "memory");
  __builtin_unreachable();
}

__attribute__((noinline)) static
void schedule(int id, int nprocs, fibril_t * frptr)
{
  struct drand48_data _buffer;

  if (frptr != _restart && frptr != _stop) {
    sync_lock(frptr->lock);

    if (frptr->count-- == 0) {
      if (frptr->stack.ptr != fibrili_deq.stack) {
        stack_reinstall(frptr);
      }

      longjmp(frptr, frptr->stack.top);
    } else {
      if (frptr->stack.ptr == fibrili_deq.stack) {
        stack_uninstall(frptr);
      }

      sync_unlock(frptr->lock);
    }
  } else {
    if (id == 0) return;
  }

  while (!_stop) {
    long victim;
    lrand48_r(&_buffer, &victim);
    victim %= nprocs - 1;
    if (victim >= id) victim += 1;

    fibril_t * frptr = deque_steal(_deqs[victim]);

    if (frptr) {
      if (!fibrili_deq.stack) fibrili_deq.stack = pool_take();

      DEBUG_DUMP(1, "steal:", (victim, "%d"), (frptr, "%p"));
      longjmp(frptr, stack_setup(frptr));
    }

    /** Force the worker to yield as a penalty for the failed steal. */
    sched_yield();
  }

  sync_barrier(nprocs);

  if (id) pthread_exit(NULL);
  else longjmp(_stop, _stop->stack.top);
}

void fibrili_init(int id, int nprocs)
{
  _tid = id;
  stack_init(id);

  if (id == 0) {
    /** Setup deque pointers. */
    _deqs = malloc(sizeof(deque_t * [nprocs]));
  }

  sync_barrier(nprocs);
  _deqs[id] = &fibrili_deq;
  sync_barrier(nprocs);

  DEBUG_DUMP(2, "proc_start:", (id, "%d"), (_deqs[id], "%p"));
  sync_barrier(nprocs);

  fibril_t fr;
  fibril_init(&fr);
  _restart = &fr;
  DEBUG_DUMP(2, "restart:", (_restart, "%p"), (_restart->stack.top, "%p"),
      (_restart->stack.btm, "%p"));
  fibrili_membar(fibrili_join(_restart));
  schedule(id, nprocs, _frptr);
}

void fibrili_exit(int id, int nprocs)
{
  fibril_t fr;

  if (id != 0) {
    fibril_init(&fr);
    _stop = &fr;
    DEBUG_DUMP(2, "proc_stop:", (_stop, "%p"), (fibrili_deq.stack, "%p"));
    fibrili_membar(fibrili_join(_stop));
  } else {
    _stop = &fr;
    sync_barrier(nprocs);
  }

  free(_deqs);
}

void fibrili_resume(fibril_t * frptr)
{
  _frptr = frptr;
  longjmp(_restart, _restart->stack.top);
}

__attribute__((noinline))
void fibrili_join(fibril_t * frptr)
{
  frptr->pc = __builtin_return_address(0);
  fibrili_resume(frptr);
}

