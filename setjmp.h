#ifndef SETJMP_H
#define SETJMP_H

#include "fibrili.h"

__attribute__((always_inline)) extern inline
int fibrili_setjmp(struct _fibril_t * frptr)
{
  register int ret asm ("eax");
  register void * ptr asm ("rcx") = &frptr->regs;

  __asm__ ( "mov\t%%rsp,(%1)\n\t"
            "mov\t%%rbp,0x8(%1)\n\t"
            "mov\t%%rbx,0x10(%1)\n\t"
            "mov\t%%r12,0x18(%1)\n\t"
            "mov\t%%r13,0x20(%1)\n\t"
            "mov\t%%r14,0x28(%1)\n\t"
            "mov\t%%r15,0x30(%1)\n\t"
            "lea\t0x6(%%rip),%%rax\n\t"
            "mov\t%%rax,0x38(%1)\n\t"
            "xor\t%0,%0\n\t"
            : "=r" (ret) : "r" (ptr)
            : "rdx","rsi","rdi","r8","r9","r10","r11" );

  return ret;
}

__attribute((noreturn, always_inline)) extern inline
void fibrili_longjmp(const struct _fibril_t * frptr, void * rsp)
{
  register const void * ptr asm ("rcx") = &frptr->regs;

  __asm__ ( "mov\t %1,%%rsp\n\t"
            "mov\t 0x8 (%0),%%rbp\n\t"
            "mov\t 0x10(%0),%%rbx\n\t"
            "mov\t 0x18(%0),%%r12\n\t"
            "mov\t 0x20(%0),%%r13\n\t"
            "mov\t 0x28(%0),%%r14\n\t"
            "mov\t 0x30(%0),%%r15\n\t"
            "mov\t $0x1,%%eax\n\t"
            "jmp\t*0x38(%0)"
            : : "r" (ptr), "r" (rsp ? rsp : frptr->regs.rsp) : "eax");
  __builtin_unreachable();
}

#endif /* end of include guard: SETJMP_H */
