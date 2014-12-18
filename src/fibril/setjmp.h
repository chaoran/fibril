#ifndef SETJMP_H
#define SETJMP_H

#include "fibrili.h"

typedef struct _fibrili_state_t {
  void * rbp;
  void * rbx;
  void * r12;
  void * r13;
  void * r14;
  void * r15;
  void * rip;
} * fibrili_state_t;

__attribute__((always_inline)) extern inline
int fibrili_setjmp(fibrili_state_t state)
{
  register int ret asm ("eax");
  register void * ptr asm ("rcx") = state;

  __asm__ ( "mov\t%%rbp,0x0(%1)\n\t"
            "mov\t%%rbx,0x8(%1)\n\t"
            "mov\t%%r12,0x10(%1)\n\t"
            "mov\t%%r13,0x18(%1)\n\t"
            "mov\t%%r14,0x20(%1)\n\t"
            "mov\t%%r15,0x28(%1)\n\t"
            "lea\t0x6(%%rip),%%rax\n\t"
            "mov\t%%rax,0x30(%1)\n\t"
            "xor\t%0,%0\n\t"
            : "=r" (ret) : "r" (ptr)
            : "rdx","rsi","rdi","r8","r9","r10","r11",
            "xmm0", "xmm1", "xmm2", "xmm3", "xmm4",
            "xmm5", "xmm6", "xmm7", "xmm8", "xmm9",
            "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
            "cc", "memory" );

  return ret;
}

__attribute((noreturn, always_inline)) extern inline
void fibrili_longjmp(const fibrili_state_t state, void * rsp)
{
  register const void * ptr asm ("rcx") = state;

  __asm__ ( "mov\t %1,%%rsp\n\t"
            "mov\t 0x0 (%0),%%rbp\n\t"
            "mov\t 0x8 (%0),%%rbx\n\t"
            "mov\t 0x10(%0),%%r12\n\t"
            "mov\t 0x18(%0),%%r13\n\t"
            "mov\t 0x20(%0),%%r14\n\t"
            "mov\t 0x28(%0),%%r15\n\t"
            "mov\t $0x1,%%eax\n\t"
            "jmp\t*0x30(%0)"
            : : "r" (ptr), "r" (rsp) : "eax");
  __builtin_unreachable();
}

#endif /* end of include guard: SETJMP_H */
