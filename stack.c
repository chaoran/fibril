#include <sys/mman.h>
#include "safe.h"
#include "param.h"
#include "stack.h"

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

