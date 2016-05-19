#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "pool.h"
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"
#include "stack.h"
#include "stats.h"

#ifdef FIBRIL_STATS
extern void * MAIN_STACK_TOP;

void handle_segfault(int s, siginfo_t * si, void * unused)
{
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

  void * stack = fibrili_deq.stack;
  void * addr = PAGE_ALIGN_DOWN(si->si_addr);
  if (addr < stack || addr >= stack + PARAM_STACK_SIZE) return;

  STATS_COUNT(N_PAGES, 1);
  SAFE_NNCALL(mprotect(addr, PARAM_PAGE_SIZE, PROT_READ | PROT_WRITE));
}
#endif

void stack_init(int id)
{
#ifdef FIBRIL_STATS
  stack_t altstack = {
    .ss_flags = 0,
    .ss_size = PARAM_STACK_SIZE
  };
  SAFE_RZCALL(posix_memalign(&altstack.ss_sp, PARAM_PAGE_SIZE,
        PARAM_STACK_SIZE));
  SAFE_NNCALL(sigaltstack(&altstack, NULL));

  struct sigaction sa = {
    .sa_flags = SA_SIGINFO | SA_STACK,
    .sa_sigaction = handle_segfault,
  };
  SAFE_NNCALL(sigaction(SIGSEGV, &sa, NULL));
#endif

  if (id == 0) {
    fibrili_deq.stack = PARAM_STACK_ADDR;

#ifdef FIBRIL_STATS
    SAFE_ASSERT(MAIN_STACK_TOP >= PARAM_STACK_ADDR);
    SAFE_ASSERT(MAIN_STACK_TOP < (PARAM_STACK_ADDR + PARAM_STACK_SIZE));
    size_t size = MAIN_STACK_TOP - PARAM_STACK_ADDR;

    STATS_COUNT(N_PAGES, ((PARAM_STACK_ADDR + PARAM_STACK_SIZE) -
          MAIN_STACK_TOP)  / PARAM_PAGE_SIZE);

    int flags = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
    SAFE_NNCALL(mmap(PARAM_STACK_ADDR, size, PROT_NONE, flags, -1, 0));
#endif
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

  if (addr) pool_put(addr);

  fibrili_deq.stack = frptr->stack.ptr;
}

