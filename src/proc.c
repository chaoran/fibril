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
static int nprocs;

void proc_resume(const fibril_t * frptr, void * rsp)
{
  DEBUG_DUMP(3, "jump:", (frptr->pc, "%p"), (rsp, "%p"));
  __asm__ ( "mov\t%1,%%rsp\n\t"
            "mov\t%0,%%rbp\n\t"
            "jmp\t*%2\n\t"
            : : "r" (frptr->stack.btm), "r" (rsp), "r" (frptr->pc) );
  __builtin_unreachable();
}

__attribute__((noreturn)) static
void execute(fibril_t * frptr)
{
  void * top = stack_setup(frptr);
  sync_unlock(frptr->lock);
  proc_resume(frptr, top);
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

__attribute__((noinline))
static void schedule(int id, int nprocs)
{
  if (_frptr == _restart) {
    sync_unlock(_frptr->lock);
    if (id == 0) return;
  } else yield(_frptr);

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

  sync_barrier(nprocs);

  if (id) pthread_exit(NULL);
  else proc_resume(_stop, _stop->stack.top);
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

  fibril_t fr;
  fibril_init(&fr);
  _restart = &fr;
  fibrili_membar(fibrili_yield(_restart));
  schedule(id, nprocs);
}

void proc_stop()
{
  fibril_t fr;

  if (_tid != 0) {
    fibril_init(&fr);
    sync_lock(fr.lock);

    _stop = &fr;
    DEBUG_DUMP(2, "proc_stop:", (_stop, "%p"), (fibrili_deq.stack, "%p"));
    fibrili_membar(fibrili_yield(_stop));
  } else {
    _stop = &fr;
    sync_barrier(param_nprocs(0));
  }

  free(_deqs);
}

void proc_yield(fibril_t * frptr)
{
  _frptr = frptr;
  proc_resume(_restart, _restart->stack.top);
}

__attribute__((noinline))
void fibrili_yield(fibril_t * frptr)
{
  frptr->pc = __builtin_return_address(0);
  proc_yield(frptr);
}

