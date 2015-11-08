#define _GNU_SOURCE
#include <strings.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"
#include "stack.h"
#include "stats.h"

static mutex_t * lock;
static int devnull;

#if STATS_STACK_PAGES != 0
void handle_segfault(int s, siginfo_t * si, void * unused)
{
  abort();
  if (s != SIGSEGV) return;

  if (si->si_code != SEGV_ACCERR) {
    struct sigaction default_action = {
      .sa_handler = SIG_DFL,
      .sa_sigaction = NULL,
      .sa_mask = 0,
      .sa_flags = 0,
      .sa_restorer = NULL
    };
    sigaction(SIGSEGV, &default_action, NULL);
    return;
  }

  STATS_COUNTER_INC(STATS_STACK_PAGES, 1);
  return;

  void * addr = si->si_addr;
  void * stack = fibrili_deq.stack;

  if (addr < stack || addr >= stack + PARAM_STACK_SIZE) return;

  void * old_top = (*(void **) stack);
  void * new_top = PAGE_ALIGN_DOWN(addr);
  size_t size = old_top - new_top;

  SAFE_ASSERT(addr < old_top);
  SAFE_NNCALL(mprotect(new_top, size, PROT_READ | PROT_WRITE));
  STATS_COUNTER_INC(STATS_STACK_PAGES, size / PARAM_PAGE_SIZE);

  *(void **) stack = new_top;
}
#endif

static unmap(struct _fibril_t * frptr)
{
  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

#if STATS_STACK_PAGES != 0
  void * old_top = *(void **) addr;
  void * new_top = addr + size;
  DEBUG_ASSERT(old_top <= new_top);
  if (new_top != old_top) {
    long diff = (long) (new_top - old_top) / PARAM_PAGE_SIZE;
    STATS_COUNTER_DEC(STATS_STACK_PAGES, diff);
  }
#endif

  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  DEBUG_DUMP(3, "unmap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));
  SAFE_NNCALL(mmap(addr, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE, devnull, 0));
}

static remap(struct _fibril_t * frptr)
{
  void * addr = frptr->stack.ptr;
  void * top  = frptr->stack.top;
  size_t size = PAGE_ALIGN_DOWN(top) - addr;

  DEBUG_ASSERT(addr != NULL && addr < top && top < addr + PARAM_STACK_SIZE);
  DEBUG_DUMP(3, "remap:", (frptr, "%p"), (addr, "%p"), (size, "0x%lx"));

#if STATS_STACK_PAGES != 0
  const int prot = PROT_NONE;
#else
  const int prot = PROT_READ | PROT_WRITE;
#endif
  const int flags = MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS | MAP_NORESERVE;
  SAFE_NNCALL(mmap(addr, size, prot, flags, -1, 0));

#if STATS_STACK_PAGES != 0
  SAFE_NNCALL(mprotect(addr, PARAM_PAGE_SIZE, PROT_READ | PROT_WRITE));
  *(void **) addr = addr + size;
#endif
}

static void * alloc(struct _fibril_t * frptr, size_t size)
{
  void * addr;
  const size_t align = PARAM_PAGE_SIZE;
  SAFE_RZCALL(posix_memalign(&addr, align, size));
  DEBUG_DUMP(3, "alloc:", (frptr, "%p"), (addr, "%p"));

#if STATS_STACK_PAGES != 0
  *(void **) addr = addr + PARAM_STACK_SIZE;

  void * protaddr = addr + PARAM_PAGE_SIZE;
  size_t protsize = PARAM_STACK_SIZE - PARAM_PAGE_SIZE;
  SAFE_NNCALL(mprotect(protaddr, protsize, PROT_NONE));
#endif

  return addr;
}

static void dealloc(struct _fibril_t * frptr, void * addr)
{
#if STATS_STACK_PAGES != 0
  void * old_top = *(void **) addr;
  long npages = (addr + PARAM_STACK_SIZE - old_top) / PARAM_PAGE_SIZE;
  STATS_COUNTER_DEC(STATS_STACK_PAGES, npages);
#endif

  DEBUG_DUMP(3, "free:", (frptr, "%p"), ((addr), "%p"));
  DEBUG_ASSERT(addr != NULL);
  free(addr);
}

void stack_init(int id)
{
  if (id) {
    fibrili_deq.stack = alloc(NULL, PARAM_STACK_SIZE);
    return;
  }

  SAFE_NNCALL(devnull = open("/dev/zero", O_RDONLY));
  fibrili_deq.stack = PARAM_STACK_ADDR;

#if STATS_STACK_PAGES != 0
  void * protaddr = PARAM_STACK_ADDR + PARAM_PAGE_SIZE;
  size_t protsize = PARAM_STACK_SIZE - PARAM_PAGE_SIZE;
  printf("protaddr = %p, protsize = %p\n", protaddr, protsize);
  SAFE_NNCALL(mprotect(protaddr, protsize, PROT_NONE));

  *(void **) fibrili_deq.stack = PARAM_STACK_ADDR + PARAM_STACK_SIZE;
#endif
  *(int *) protaddr = 1;
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

  unmap(frptr);
  fibrili_deq.stack = alloc(frptr, PARAM_STACK_SIZE);

  mutex_unlock(&lock, &mutex);
  return 1;
}

void stack_reinstall(struct _fibril_t * frptr)
{
  DEBUG_ASSERT(frptr != NULL);

  mutex_t mutex;
  mutex_lock(&lock, &mutex);

  dealloc(frptr, fibrili_deq.stack);
  fibrili_deq.stack = frptr->stack.ptr;
  remap(frptr);

  mutex_unlock(&lock, &mutex);
}

