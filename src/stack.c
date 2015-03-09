#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"
#include "stack.h"

static mutex_t * lock;

void stack_init(int id)
{
  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;
  } else {
    const size_t align = PARAM_PAGE_SIZE;
    const size_t size  = PARAM_STACK_SIZE;

    SAFE_RZCALL(posix_memalign(&fibrili_deq.stack, align, size));
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

  mutex_t mutex;
  if (!mutex_trylock(&lock, &mutex)) return 0;

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "uninstall:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(munmap(addr, size));

  const size_t align = PARAM_PAGE_SIZE;
  SAFE_RZCALL(posix_memalign(&fibrili_deq.stack, align, PARAM_STACK_SIZE));
  DEBUG_DUMP(3, "alloc:", (frptr, "%p"), (fibrili_deq.stack, "%p"));

  mutex_unlock(&lock, &mutex);
  return 1;
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  mutex_t mutex;
  mutex_lock(&lock, &mutex);

  DEBUG_DUMP(3, "free:", (frptr, "%p"), (fibrili_deq.stack, "%p"));
  DEBUG_ASSERT(fibrili_deq.stack != NULL);
  free(fibrili_deq.stack);
  fibrili_deq.stack = addr;

  const int prot = PROT_READ | PROT_WRITE;
  const int flags = MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS | MAP_NORESERVE;
  DEBUG_DUMP(3, "reinstall:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  SAFE_NNCALL(mmap(addr, size, prot, flags, -1, 0));

  mutex_unlock(&lock, &mutex);
}

