#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "param.h"
#include "stack.h"

void * stack_setup(fibril_t * frptr)
{
  size_t align = PARAM_PAGE_SIZE;
  size_t size  = PARAM_STACK_SIZE;
  SAFE_RZCALL(posix_memalign(&fibrili_deq.stack, align, size));
  SAFE_ASSERT(fibrili_deq.stack != NULL && PAGE_ALIGNED(fibrili_deq.stack));

  void ** rsp = fibrili_deq.stack + size;
  return rsp;
}

void stack_uninstall(fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "munmap:", (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(munmap(addr, size));
}

void stack_reinstall(fibril_t * frptr)
{
  static int prot = PROT_READ | PROT_WRITE;
  static int flag = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "mmap:", (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(mmap(addr, size, prot, flag, -1, 0));
}

