#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "param.h"
#include "stack.h"

void * stack_setup(struct _fibril_t * frptr)
{
  size_t size  = PARAM_STACK_SIZE;

  if (fibrili_deq.stack == NULL) {
    size_t align = PARAM_PAGE_SIZE;
    SAFE_RZCALL(posix_memalign(&fibrili_deq.stack, align, size));
    SAFE_ASSERT(fibrili_deq.stack != NULL && PAGE_ALIGNED(fibrili_deq.stack));
    DEBUG_DUMP(3, "stack:", (fibrili_deq.stack, "%p"));
  }

  void ** rsp = fibrili_deq.stack + size;

  /** Reserve 128 byte at the bottom. */
  rsp -= 16;

  return rsp;
}

void stack_uninstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);
  if (frptr->stack.ptr != fibrili_deq.stack) return;

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "munmap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(munmap(addr, size));

  /** Mark current stack as lost. */
  fibrili_deq.stack = NULL;
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);
  if (frptr->stack.ptr == fibrili_deq.stack) return;

  static int prot = PROT_READ | PROT_WRITE;
  static int flag = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "mmap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(mmap(addr, size, prot, flag, -1, 0));
}

