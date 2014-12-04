#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "param.h"
#include "stack.h"

static void * _trampoline;

void stack_init()
{
  if (_trampoline == NULL) {
    fibrili_deq.stack = PAGE_ALIGN_DOWN(PARAM_STACK_ADDR);
    _trampoline = &&TRAMPOLINE;
  } else {
TRAMPOLINE:
    __asm__ ( /** Restore stack pointer. */
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

void * stack_setup(fibril_t * frptr, void * stack)
{
  size_t align = PARAM_PAGE_SIZE;
  size_t size  = PARAM_STACK_SIZE;
  SAFE_RZCALL(posix_memalign(&fibrili_deq.stack, align, size));
  SAFE_ASSERT(fibrili_deq.stack != NULL && PAGE_ALIGNED(fibrili_deq.stack));

  void ** top = frptr->regs.rsp;
  void ** btm = frptr->regs.rbp;
  void ** rsp = fibrili_deq.stack + size;

  /**
   * Store the return address, parent's frame pointer, and the
   * stolen frame's pointer at the bottom of the stack.
   */
  *(--rsp) = stack;
  *(--rsp) = *(btm + 1);
  *(--rsp) = *(btm);
  *(--rsp) = btm;

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
  return rsp;
}

void stack_uninstall(fibril_t * frptr)
{
  void * stack = fibrili_deq.stack;
  frptr->stack = stack;

  void * rsp = frptr->regs.rsp;
  DEBUG_ASSERT(rsp > stack && rsp < stack + PARAM_STACK_SIZE);

  size_t size = PAGE_ALIGN_DOWN(rsp) - stack;
  SAFE_NNCALL(munmap(stack, size));
}

void stack_reinstall(fibril_t * frptr)
{
  void * stack = frptr->stack;
  fibrili_deq.stack = stack;

  void * rsp = frptr->regs.rsp;
  DEBUG_ASSERT(rsp > stack && rsp < stack + PARAM_STACK_SIZE);

  size_t size = PAGE_ALIGN_DOWN(rsp) - stack;
  int prot = PROT_READ | PROT_WRITE;
  int flag = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;
  SAFE_NNCALL(mmap(stack, size, prot, flag, -1, 0));
}

