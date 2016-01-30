#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "pool.h"
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"
#include "stack.h"

void stack_init(int id)
{
  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;
  }
}

void * stack_setup(struct _fibril_t * frptr)
{
  void ** rsp = fibrili_deq.stack + PARAM_STACK_SIZE;

  /** Reserve 128 byte at the bottom. */
  rsp -= 16;
  return rsp;
}

int stack_uninstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  void * addr = frptr->stack.ptr;
  fibrili_deq.stack = NULL;

  if (addr != PARAM_STACK_ADDR) {
    size_t size = PAGE_ALIGN_DOWN(frptr->stack.top) - addr;
    SAFE_NNCALL(madvise(addr, size, MADV_DONTNEED));
  }

  return 1;
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  void * addr = fibrili_deq.stack;
  SAFE_ASSERT(addr != PARAM_STACK_ADDR);

  if (addr) {
    SAFE_NNCALL(madvise(addr, PARAM_STACK_SIZE, MADV_DONTNEED));
    pool_put(addr);
  }

  fibrili_deq.stack = frptr->stack.ptr;
}

