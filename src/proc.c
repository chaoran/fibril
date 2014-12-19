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

void proc_resume(const fibrili_state_t state, void * rsp)
{
  register void * ret asm ("eax");

  __asm__ __volatile__ ( "mov\t %2,%%rsp\n\t"
            "mov\t 0x0 (%1),%%rbp\n\t"
            "mov\t 0x8 (%1),%%rbx\n\t"
            "mov\t 0x10(%1),%%r12\n\t"
            "mov\t 0x18(%1),%%r13\n\t"
            "mov\t 0x20(%1),%%r14\n\t"
            "mov\t 0x28(%1),%%r15\n\t"
            "mov\t $0x1,%0\n\t"
            "jmp\t*0x30(%1)"
            : "=&r" (ret) : "r" (state), "r" (rsp) );
  __builtin_unreachable();
}

__attribute__((noreturn)) static
void execute(fibril_t * frptr)
{
  void * top = stack_setup(frptr);
  sync_unlock(frptr->lock);
  proc_resume(&frptr->state, top);
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
  else proc_resume(&_stop->state, _stop->stack.top);
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
    sync_lock(fr.lock);

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
  proc_resume(&_restart->state, _restart->stack.top);
}

