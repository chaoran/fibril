#include <stdlib.h>
#include <pthread.h>
#include "sched.h"
#include "deque.h"
#include "atomic.h"

#define STACK_SIZE (8 * 1024 * 1024)

int _nprocs;
__thread int _tid;

static __thread deque_t ** _deqs;
static void * _stack;
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
  void * stack = malloc(STACK_SIZE);
  frptr->stack = stack;

  __asm__ ( "mov\t%0,%%rsp" : : "g" (stack + STACK_SIZE) );
  LONGJMP(frptr);
}

__attribute__((noreturn)) static
void schedule(fibril_t * frptr, int id, int nprocs)
{
  /** Unlock after switch to scheduler stack. */
  if (frptr) atomic_unlock(frptr->lock);

  while (atomic_load(_frptr) == NULL) {
    int victim = rand() % nprocs;

    if (victim == id) {
      frptr = deque_steal(_deqs[victim]);

      if (frptr) {
        execute(frptr);
      }
    }
  }

  if (id) pthread_exit(NULL);
  else sched_resume(_frptr);

  __builtin_unreachable();
}

void sched_restart(fibril_t * frptr)
{
  /** Change to scheduler stack. */
  __asm__ ( "mov\t%0,%%rsp" : : "g" (_stack) );

  schedule(frptr, _tid, _nprocs);
}

void sched_start(int id, int nprocs)
{
  /** Setup the scheduler stack. */
  register void * rsp asm ("rsp");
  _stack = rsp;

  /** Setup deque pointers. */
  if (id == 0) {
    _deqs = malloc(sizeof(deque_t * [nprocs]));
  }

  atomic_barrier(nprocs);
  _deqs[id] = &fibrili_deq;

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
  }

AFTER_YIELD:
  free(_deqs);
}

void sched_resume(const fibril_t * frptr)
{
  __asm__ ( "mov\t%0,%%rsp" : : "g" (frptr->regs.rsp) );
  LONGJMP(frptr);
}

