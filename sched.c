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
static void * _trampoline;

__attribute__((noreturn)) static
void execute(frame_t fm)
{
  void * rsp = stack_setup(fm.frptr, fm.stack, _trampoline);
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
    fibrili_deq.stack = PAGE_ALIGN_DOWN(PARAM_STACK_ADDR);

    /** Setup deque pointers. */
    _deqs = malloc(sizeof(deque_t * [nprocs]));

    /** Setup trampoline segment. */
    if (_trampoline == NULL) {
      _trampoline = &&TRAMPOLINE;
    } else {
TRAMPOLINE: __asm__ ("nop" : : : "rax", "rbx", "rcx", "rdx", "rdi", "rsi",
                "r11", "r12", "r13", "r14", "r15");
      __asm__ (
                /** Restore stack pointer. */
                "mov\t(%%rbp),%%rsp\n\t"
                /** Restore parent's frame pointer. */
                "mov\t0x8(%%rbp),%%rcx\n\t"
                "mov\t%%rcx,(%%rsp)\n\t"
                /** Restore return address. */
                "mov\t0x10(%%rbp),%%rcx\n\t"
                "mov\t%%rcx,0x8(%%rsp)\n\t"
                /** Update current stack. */
                "mov\t0x18(%%rbp),%%rcx\n\t"
                "mov\t%%rcx,%1\n\t"
                /** Compute stack address. */
                "lea\t0x20(%%rbp),%%rdi\n\t"
                "sub\t%0,%%rdi\n\t"
                /** Free the stack. */
                "push\t%%rax\n\t"
                "call\tfree\n\t"
                "pop\t%%rax\n\t"
                /** Return to parent. */
                "pop\t%%rbp\n\t"
                "ret\n\t"
                : : "m" (PARAM_STACK_SIZE), "m" (fibrili_deq.stack)
                : "rax", "rcx", "rdx"
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

