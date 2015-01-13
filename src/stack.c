#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "param.h"
#include "stack.h"

static void * stack_alloc()
{
  const size_t align = PARAM_PAGE_SIZE;
  const size_t size = PARAM_STACK_SIZE;

  void * addr;
  SAFE_RZCALL(posix_memalign(&addr, align, size));
  SAFE_ASSERT(addr != NULL && PAGE_ALIGNED(addr));

  DEBUG_DUMP(3, "stack_alloc:", (addr, "%p"));
  return addr;
}

void stack_init(int id)
{
  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;
  } else {
    fibrili_deq.stack = stack_alloc();
  }
}

void * stack_setup(struct _fibril_t * frptr)
{
  void ** rsp = fibrili_deq.stack + PARAM_STACK_SIZE;

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
  fibrili_deq.stack = stack_alloc();
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);
  void * stack = fibrili_deq.stack;

  if (frptr->stack.ptr == stack) return;

  free(stack);
  fibrili_deq.stack = frptr->stack.ptr;

  static int prot = PROT_READ | PROT_WRITE;
  static int flag = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "mmap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(mmap(addr, size, prot, flag, -1, 0));
}

