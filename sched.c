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
static void * _trampoline;

__attribute__((noreturn, noinline)) static
void execute(fibril_t * frptr)
{
  size_t size = PARAM_STACK_SIZE;
  void * stack = malloc(size);

  frptr->stack = stack;

  void ** top = frptr->regs.rsp;
  void ** btm = frptr->regs.rbp;

  sync_unlock(frptr->lock);

  void ** rsp = stack + size;

  *(--rsp) = btm + 1;
  *(--rsp) = _trampoline;

  switch (btm - top > 5 ? 5 : btm - top) {
    case 5: *(--rsp) = *btm--;
    case 4: *(--rsp) = *btm--;
    case 3: *(--rsp) = *btm--;
    case 2: *(--rsp) = *btm--;
    case 1: *(--rsp) = *btm--;
    case 0: *(--rsp) = *btm--;
  }

  rsp -= btm - top + 1;

  DEBUG_DUMP(2, "execute:", (frptr, "%p"), (stack, "%p"), (rsp, "%p"));
  fibrili_longjmp(frptr, rsp);
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

  if (id == 0) {
    /** Setup deque pointers. */
    _deqs = malloc(sizeof(deque_t * [nprocs]));

    /** Setup trampoline segment. */
    if (_trampoline == NULL) {
      _trampoline = &&TRAMPOLINE;
    } else {
TRAMPOLINE: __asm__ (
                "lea\t0x8(%%rsp),%%rdi\n\t"
                "sub\t%0,%%rdi\n\t"
                "pop\t%%rsp\n\t"
                "push\t%%rax\n\t"
                "call\tfree\n\t"
                "pop\t%%rax\n\t"
                "ret\n\t" : : "m" (PARAM_STACK_SIZE)
            );
    }
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
    fibrili_longjmp(frptr, NULL);
  } else {
    fibrili_yield(frptr);
  }
}

