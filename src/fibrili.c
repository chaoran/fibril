#include <stdlib.h>
#include <pthread.h>
#include "fifo.h"
#include "sync.h"
#include "stack.h"
#include "debug.h"
#include "deque.h"
#include "param.h"
#include "fibrile.h"

static __thread fibril_t * _restart;
static __thread fibril_t * _frptr;
static __thread fifo_handle_t _handle;
static deque_t ** _deqs;
static fibril_t * _stop;
static fifo_t _fifo;

__attribute__((noreturn)) static
void longjmp(fibril_t * frptr, void * rsp)
{
  DEBUG_DUMP(3, "jump:", (frptr->pc, "%p"), (rsp, "%p"));
  sync_unlock(frptr->lock);
  __asm__ ( "mov\t%1,%%rsp\n\t"
            "mov\t%0,%%rbp\n\t"
            "jmp\t*%2\n\t"
            : : "r" (frptr->stack.btm), "r" (rsp), "r" (frptr->pc) );
  __builtin_unreachable();
}

__attribute__((noinline)) static
void schedule(int id, int nprocs, fibril_t * frptr)
{
  if (frptr != _restart && frptr != _stop) {
    sync_lock(frptr->lock);

    if (frptr->count-- == 0) {
      if (frptr->stack.ptr != fibrili_deq.stack) {
        if (frptr->unmapped) {
          stack_reinstall(frptr);
          frptr->unmapped = 0;
        }
        else {
          fifo_put(&_fifo, &_handle, fibrili_deq.stack);
          fibrili_deq.stack = frptr->stack.ptr;
        }
      }

      longjmp(frptr, frptr->stack.top);
    } else {
      if (frptr->stack.ptr == fibrili_deq.stack) {
        if (stack_uninstall(frptr)) {
          frptr->unmapped = 1;
          sync_unlock(frptr->lock);
        }
        else {
          sync_unlock(frptr->lock);
          fibrili_deq.stack = fifo_get(&_fifo, &_handle);
        }
      } else {
        sync_unlock(frptr->lock);
      }
    }
  } else {
    if (id == 0) return;
  }

  while (sync_load(_stop) == NULL) {
    int victim = rand() % nprocs;

    if (victim != id) {
      fibril_t * frptr = deque_steal(_deqs[victim]);

      if (frptr) {
        DEBUG_DUMP(1, "steal:", (victim, "%d"), (frptr, "%p"));
        longjmp(frptr, stack_setup(frptr));
      }
    }
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
    fifo_init(&_fifo, 510, nprocs);
    _deqs = malloc(sizeof(deque_t * [nprocs]));
  }

  sync_barrier(nprocs);
  _deqs[id] = &fibrili_deq;
  sync_barrier(nprocs);

  fifo_register(&_fifo, &_handle);
  DEBUG_DUMP(2, "proc_start:", (id, "%d"), (_deqs[id], "%p"));

  fibril_t fr;
  fibril_init(&fr);
  _restart = &fr;
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

