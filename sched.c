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

  /**
   * Store the return address, parent's frame pointer, and the
   * stolen frame's pointer at the bottom of the stack.
   */
  *(--rsp) = *(btm + 1);  /** Copy return address. */
  *(--rsp) = *(btm);      /** Copy caller's base pointer. */
  *(--rsp) = btm;         /** Copy base pointer. */

  /**
   * Use the trampoline as return address and the pointer to the saved
   * variables on the new stack as parent's frame pointer.
   */
  void * tmp = rsp;

  *(--rsp) = _trampoline;
  *(--rsp) = tmp;

  *(btm + 1) = _trampoline;
  *(btm)     = tmp;

  /** Copy the saved parent's registers to the new stack. */
  switch (btm - top > 5 ? 5 : btm - top) {
    case 5: *(--rsp) = *(--btm);
    case 4: *(--rsp) = *(--btm);
    case 3: *(--rsp) = *(--btm);
    case 2: *(--rsp) = *(--btm);
    case 1: *(--rsp) = *(--btm);
  }

  /** Adjust the stack pointer to keep correct offset to the frame pointer. */
  rsp -= btm - top;

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
                /** Restore stack pointer. */
                "mov\t(%%rbp),%%rsp\n\t"
                /** Restore parent's frame pointer. */
                "mov\t0x8(%%rbp),%%rcx\n\t"
                "mov\t%%rcx,(%%rsp)\n\t"
                /** Restore return address. */
                "mov\t0x10(%%rbp),%%rcx\n\t"
                "mov\t%%rcx,0x8(%%rsp)\n\t"
                /** Compute stack address. */
                "lea\t0x18(%%rbp),%%rdi\n\t"
                "sub\t%0,%%rdi\n\t"
                /** Free the stack. */
                "push\t%%rax\n\t"
                "call\tfree\n\t"
                "pop\t%%rax\n\t"
                /** Return to parent. */
                "pop\t%%rbp\n\t"
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

