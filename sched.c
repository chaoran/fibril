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

#define LONGJMP(frptr) do { \
  __asm__ ( "mov\t 0x8 (%0),%%rbp\n\t" \
            "mov\t 0x10(%0),%%rbx\n\t" \
            "mov\t 0x18(%0),%%r12\n\t" \
            "mov\t 0x20(%0),%%r13\n\t" \
            "mov\t 0x28(%0),%%r14\n\t" \
            "mov\t 0x30(%0),%%r15\n\t" \
            "jmp\t*0x38(%0)" : : "r" (&(frptr)->regs) \
            : "rbx", "r12", "r13", "r14", "r15" ); \
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

void schedule(fibril_t * frptr)
{
  fibril_t fr;

  if (frptr == NULL) {
    fibril_init(&fr);
    fibrili_save(&fr);
    fr.regs.rip = &&RESTART;

    _restart = &fr;

    if (_tid == 0) return;
  } else {
    sync_unlock(frptr->lock);
  }

  while (sync_load(_stop) == NULL) {
RESTART: fibrili_flush();
    int victim = rand() % PARAM_NUM_PROCS;

    if (victim != _tid) {
      frptr = deque_steal(_deqs[victim]);

      if (frptr) {
        DEBUG_DUMP(1, "steal:", (victim, "%d"), (frptr, "%p"));
        execute(frptr);
      }
    }
  }

  sync_barrier(PARAM_NUM_PROCS);

  if (_tid) pthread_exit(NULL);
  else sched_resume(_stop);
}

void sched_restart(fibril_t * frptr)
{
  sched_resume(_restart);
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

  schedule(NULL);
}

void sched_stop()
{
  fibril_t fr;

  DEBUG_DUMP(2, "sched_stop:");

  if (_tid != 0) {
    fibril_init(&fr);
    fibrili_save(&fr);
    fr.regs.rip = &&AFTER_YIELD;

    _stop = &fr;
    sched_restart(_stop);
  } else {
    _stop = &fr;
    sync_barrier(PARAM_NUM_PROCS);
  }

AFTER_YIELD: fibrili_flush();
  free(_deqs);
}

void sched_resume(const fibril_t * frptr)
{
  __asm__ ( "mov\t%0,%%rsp" : : "g" (frptr->regs.rsp) );
  LONGJMP(frptr);
}

