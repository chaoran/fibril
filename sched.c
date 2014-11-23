#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "sched.h"
#include "shmap.h"
#include "tlmap.h"

static deque_t ** _deqs;
static int __fibril_shared__ _done;
static fibril_t __fibril_shared__ _exit_fr;

joint_t * __fibril_shared__ _jtptr;

__attribute__((noreturn, noinline))
static void resume(joint_t * jtptr, const fibril_t * frptr)
{
  void * dest = jtptr->stptr->top;
  void * addr = jtptr->stptr->top + jtptr->stptr->off;

  DEBUG_ASSERT(__builtin_frame_address(0) < dest);
  DEBUG_DUMP(3, "import:", (jtptr, "%p"), (dest, "%p"), (addr, "%p"));

  if (jtptr->count < 0) {
    memcpy(dest, addr, jtptr->stptr->btm - dest);
    free(addr);

    fibrile_deq.jtptr = jtptr->parent;
    free(jtptr);
  } else {
    memcpy(dest, addr, STACK_BOTTOM - dest);

    fibrile_deq.jtptr = jtptr;
    unlock(&jtptr->lock);
  }

  DEBUG_DUMP(2, "jump:", (frptr, "%p"), (frptr->rip, "%p"));
  DEBUG_ASSERT(frptr->rip != NULL);

  __asm__ (
      "mov\t%0,%%rsp\n\t"
      "mov\t%1,%%rbp\n\t"
      "mov\t%2,%%rbx\n\t"
      "mov\t%3,%%r12\n\t"
      "mov\t%4,%%r13\n\t"
      "mov\t%5,%%r14\n\t"
      "mov\t%6,%%r15\n\t"
      "jmp\t*(%7)" : :
      "g" (frptr->rsp), "g" (frptr->rbp),
      "g" (frptr->rbx), "g" (frptr->r12),
      "g" (frptr->r13), "g" (frptr->r14),
      "g" (frptr->r15), "g" (&frptr->rip)
  );

  unreachable();
}

void sched_init(int nprocs)
{
  /** Setup deque pointers. */
  _deqs = malloc(sizeof(deque_t * [nprocs]));
  SAFE_ASSERT(_deqs != NULL);

  int i;
  for (i = 0; i < nprocs; ++i) {
    _deqs[i] = (void *) &fibrile_deq + TLS_OFFSETS[i];
  }

  /** Setup initial joint. */
  _jtptr = malloc(sizeof(joint_t));
  _jtptr->lock = 1;
  _jtptr->count = -1;
  _jtptr->stack.top = STACK_BOTTOM;
  _jtptr->stack.btm = STACK_BOTTOM;
  _jtptr->stack.off = STACK_OFFSETS[0];
  _jtptr->stptr = &_jtptr->stack;

  fibrile_deq.jtptr = _jtptr;
  lock(&_done);
}

void sched_work(int me, int nprocs)
{
  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = deque_steal(_deqs[victim], victim);

    if (frptr == NULL) continue;

    resume(frptr->jtp, frptr);
  }

  unlock(&_done);

  if (me) {
    barrier();
    exit(0);
  } else {
    DEBUG_ASSERT(_jtptr->count < 0 && _jtptr->lock == 0);
    resume(_jtptr, &_exit_fr);
  }
}

void sched_exit()
{
  if (TID == 0) {
    unlock(&_done);
  } else {
    fibril_make(&_exit_fr);

    fibrile_save(&_exit_fr, &&AFTER_EXIT);
    _jtptr->stptr->top = _exit_fr.rsp;
    _exit_fr.jtp = _jtptr;

    unlock(&_done);
    fibrile_yield(&_exit_fr);
  }

AFTER_EXIT:
  barrier();
  return;
}

__attribute__((noreturn, optimize("-fno-omit-frame-pointer")))
void sched_resume(joint_t * jtptr, const fibril_t * frptr)
{
  DEBUG_ASSERT(jtptr == fibrile_deq.jtptr);

  register void * rsp asm ("rsp");
  void * dest = jtptr->stptr->top;

  if (rsp > dest) {
    rsp = dest;
  }

  resume(jtptr, frptr);
}

