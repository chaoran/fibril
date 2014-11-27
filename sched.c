#include <stdlib.h>
#include <pthread.h>
#include "sync.h"
#include "sched.h"
#include "debug.h"
#include "deque.h"
#include "param.h"

__thread int _tid;

static __thread void * _stack;
static deque_t ** _deqs;
static fibril_t * _frptr;

#define LONGJMP(frptr) do { \
  __asm__ ( "mov\t 0x8 (%0),%%rbp\n\t" \
            "mov\t 0x10(%0),%%rbx\n\t" \
            "mov\t 0x18(%0),%%r12\n\t" \
            "mov\t 0x20(%0),%%r13\n\t" \
            "mov\t 0x28(%0),%%r14\n\t" \
            "mov\t 0x30(%0),%%r15\n\t" \
            "jmp\t*0x38(%0)" : : "g" (&(frptr)->regs) ); \
  __builtin_unreachable(); \
} while (0)

__attribute__((noreturn, noinline)) static
void execute(fibril_t * frptr)
{
  size_t size = PARAM_STACK_SIZE;
  void * stack = malloc(size);

  frptr->stack = stack;
  sync_unlock(frptr->lock);

  DEBUG_DUMP(2, "execute:", (frptr, "%p"), (stack, "%p"));

  __asm__ ( "mov\t%0,%%rsp" : : "g" (stack + size) );
  LONGJMP(frptr);
}

__attribute__((noreturn)) static
void schedule(fibril_t * frptr, int id, int nprocs)
{
  /** Unlock after switch to scheduler stack. */
  if (frptr) sync_unlock(frptr->lock);

  while (sync_load(_frptr) == NULL) {
    int victim = rand() % nprocs;

    if (victim != id) {
      frptr = deque_steal(_deqs[victim]);

      if (frptr) {
        DEBUG_DUMP(1, "steal:", (victim, "%d"), (frptr, "%p"));
        execute(frptr);
      }
    }
  }

  sync_barrier(nprocs);

  if (id) pthread_exit(NULL);
  else sched_resume(_frptr);

  __builtin_unreachable();
}

void sched_restart(fibril_t * frptr)
{
  /** Change to scheduler stack. */
  __asm__ ( "mov\t%0,%%rsp" : : "g" (_stack) );

  schedule(frptr, _tid, PARAM_NUM_PROCS);
}

void sched_start(int id, int nprocs)
{
  _tid = id;

  /** Setup the scheduler stack. */
  register void * rsp asm ("rsp");
  _stack = rsp;

  /** Setup deque pointers. */
  if (id == 0) {
    _deqs = malloc(sizeof(deque_t * [nprocs]));
  }

  sync_barrier(nprocs);
  _deqs[id] = &fibrili_deq;
  sync_barrier(nprocs);

  DEBUG_DUMP(2, "sched_start:", (id, "%d"), (_deqs[id], "%p"));

  if (id != 0) sched_restart(NULL);
}

void sched_stop()
{
  fibril_t fr;

  if (_tid != 0) {
    fibril_init(&fr);
    fibrili_save(&fr, AFTER_YIELD);

    _frptr = &fr;
    sched_restart(&fr);
  } else {
    _frptr = &fr;
    sync_barrier(PARAM_NUM_PROCS);
  }

AFTER_YIELD:
  free(_deqs);
}

void sched_resume(const fibril_t * frptr)
{
  __asm__ ( "mov\t%0,%%rsp" : : "g" (frptr->regs.rsp) );
  LONGJMP(frptr);
}

