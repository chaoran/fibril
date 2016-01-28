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

static mutex_t * volatile MMAP_LOCK;
static int devnull;

void stack_init(int id)
{
  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;
    SAFE_NNCALL(devnull = open("/dev/zero", O_RDONLY));
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

  if (addr != PARAM_STACK_ADDR) {
    size_t size = PAGE_ALIGN_DOWN(frptr->stack.top) - addr;
    SAFE_NNCALL(madvise(addr, size, MADV_DONTNEED));
    fibrili_deq.stack = pool_take(1);
  } else {
    do fibrili_deq.stack = pool_take(0);
    while (fibrili_deq.stack == NULL);
  }

  return 1;
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  void * addr = fibrili_deq.stack;
  SAFE_ASSERT(addr != PARAM_STACK_ADDR);

  pool_put(fibrili_deq.stack);

  fibrili_deq.stack = frptr->stack.ptr;
}

