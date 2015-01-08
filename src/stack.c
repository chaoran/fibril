#define _GNU_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "param.h"
#include "stack.h"

static int mmap_prot = PROT_READ | PROT_WRITE;
static int mmap_flag = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;

void stack_init(int id)
{
  if (id != 0) {
    fibrili_deq.stack = NULL;
    return;
  }

  FILE * maps = fopen("/proc/self/maps", "r");
  SAFE_ASSERT(maps != NULL);

  void * start;
  void * end;
  char path[PATH_MAX];
  char * format = "%p-%p %*s %*x %*d:%*d %*d %[[/]%s%*[ (deleted)]\n";

  while (EOF != fscanf(maps, format, &start, &end, path, path + 1)
      && !strstr(path, "[stack]"));

  SAFE_NNCALL(fclose(maps));
  fibrili_deq.stack = start;

  /*size_t size = start - fibrili_deq.stack;*/
  /*SAFE_NNCALL(mmap(fibrili_deq.stack, size, mmap_prot, mmap_flag, -1, 0));*/
}

static void * alloc()
{
  const size_t align = PARAM_PAGE_SIZE;
  const size_t size  = PARAM_STACK_SIZE;

  void * addr;

  SAFE_RZCALL(posix_memalign(&addr, align, size));
  SAFE_ASSERT(addr != NULL && PAGE_ALIGNED(addr));
  DEBUG_DUMP(3, "stack_alloc:", (addr, "%p"));

  return addr;
}

void * stack_setup(struct _fibril_t * frptr)
{
  size_t size  = PARAM_STACK_SIZE;

  if (fibrili_deq.stack == NULL) {
    fibrili_deq.stack = alloc();
  }

  void ** rsp = fibrili_deq.stack + size;

  /** Reserve 128 byte at the bottom. */
  rsp -= 16;

  return rsp;
}

void stack_uninstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "munmap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  /*SAFE_NNCALL(munmap(addr, size));*/
  /*fibrili_deq.stack = NULL;*/

  addr += PARAM_PAGE_SIZE;
  size -= PARAM_PAGE_SIZE;

  /*void * stack = fibrili_deq.stack = alloc();*/

  fibrili_deq.stack = mremap(addr, size, PARAM_STACK_SIZE, MREMAP_MAYMOVE);
  SAFE_ASSERT(fibrili_deq.stack != MAP_FAILED && fibrili_deq.stack != addr);
}

void stack_reinstall(struct _fibril_t * frptr)
{
  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_DUMP(3, "mmap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  /*SAFE_NNCALL(mmap(addr, size, mmap_prot, mmap_flag, -1, 0));*/

  SAFE_NNCALL(mremap(addr, PARAM_PAGE_SIZE, size, 0));
}

