#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/version.h>
#include "safe.h"
#include "sync.h"
#include "param.h"
#include "stack.h"

typedef struct _stack_t {
  struct _stack_t * next;
  void * addr;
} stack_t;

static __thread stack_t * _stacks;

static void stack_move(void * dest, size_t dest_len, void * src, size_t src_len)
{
  static const int prot = PROT_READ | PROT_WRITE;
  static const int remap_flags = MREMAP_MAYMOVE | MREMAP_FIXED;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
  static const int map_flags = MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS
    | MAP_NORESERVE | MAP_GROWSDOWN | MAP_STACK;
#else
  static const int map_flags = MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS
    | MAP_NORESERVE | MAP_GROWSDOWN;
#endif

  if (MAP_FAILED == mremap(src, src_len, dest_len, remap_flags, dest)) {
    SAFE_NNCALL(munmap(src, src_len));
    SAFE_NNCALL(mmap(dest, dest_len, prot, map_flags, -1, 0));
  }
}

static void * stack_alloc(void * src, size_t len)
{
  const size_t size = PARAM_STACK_SIZE;

  void * addr;
  stack_t * stack;

  if (NULL != (stack = _stacks)) {
    _stacks = stack->next;
    addr = stack->addr;
    free(stack);
    stack_move(addr, size, src, len);
  } else {
    const size_t align = PARAM_PAGE_SIZE;

    SAFE_RZCALL(posix_memalign(&addr, align, size));
    SAFE_ASSERT(addr != NULL && PAGE_ALIGNED(addr));
    DEBUG_DUMP(3, "stack_alloc:", (addr, "%p"));
  }

  return addr;
}

static void stack_free(void * addr, void * dest, size_t len)
{
  stack_move(dest, len, addr, PARAM_STACK_SIZE);

  stack_t * stack;
  SAFE_NZCALL(stack = malloc(sizeof(stack_t)));

  stack->next = _stacks;
  stack->addr = addr;

  _stacks = stack;
}

void stack_init(int id)
{
  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;
  } else {
    fibrili_deq.stack = stack_alloc(NULL, 0);
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

  DEBUG_DUMP(3, "uninstall:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  fibrili_deq.stack = stack_alloc(addr, size);
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);
  if (frptr->stack.ptr == fibrili_deq.stack) return;

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "reinstall:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);

  stack_free(fibrili_deq.stack, addr, size);
  fibrili_deq.stack = frptr->stack.ptr;
}

