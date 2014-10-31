#ifndef SCHED_H
#define SCHED_H

#include "util.h"
#include "fibril.h"

static inline __attribute__ ((noreturn))
void sched_restart()
{
  frame_t * base = fibrile_deq.base;
  DEBUG_PRINTV("restart: base=%p rbp=%p rip=%p\n", base, base->rsp, base->rip);
  SAFE_ASSERT(base->rsp != NULL && base->rip != NULL);

  __asm__ __volatile__ (
      "mov\t$0,%%eax\n\t"
      "mov\t%0,%%rsp\n\t"
      "pop\t%%rbp\n\t"
      "retq" : : "g" (fibrile_deq.base) : "eax"
  );

  unreachable();
}

static inline __attribute__ ((noreturn))
void sched_resume(const fibril_t * frptr)
{
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

int  sched_work();
void sched_exit();

#define SCHED_DONE 1

#endif /* end of include guard: SCHED_H */
