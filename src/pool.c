#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"
#include "stats.h"

#ifndef POOL_GLOBAL_SIZE
#define POOL_GLOBAL_SIZE (2048 - 3)
#endif

#ifndef POOL_LOCAL_SIZE
#define POOL_LOCAL_SIZE 7
#endif

#ifndef POOL_CACHE_SIZE
#define POOL_CACHE_SIZE 4
#endif

static struct {
  mutex_t * volatile lock;
  size_t volatile avail;
  void * buff[POOL_GLOBAL_SIZE];
} _pg __attribute__((aligned(128)));

static __thread struct {
  size_t volatile avail;
  void * buff[POOL_LOCAL_SIZE];
} _pl __attribute__((aligned(128)));

/**
 * Take a stack from the pool or allocate from heap if the pool is empty.
 * @return Return a stack or NULL if the pool has reached its limit.
 */
void * pool_take()
{
  void * stack = NULL;

  /** Take a stack from the available stacks. */
  if (_pl.avail > 0) {
    stack = _pl.buff[--_pl.avail];
  } else {
    /** Take a stack from the parent pool. */
    if (_pg.avail > 0) {
      mutex_t mutex;
      mutex_lock(&_pg.lock, &mutex);

      if (_pg.avail > 0) {
        stack = _pg.buff[--_pg.avail];
      }

      mutex_unlock(&_pg.lock, &mutex);
    }

    if (!stack) {
      SAFE_RZCALL(posix_memalign(&stack, PARAM_PAGE_SIZE, PARAM_STACK_SIZE));
      STATS_INC(N_STACKS, 1);

#ifdef FIBRIL_STATS
      SAFE_NNCALL(mprotect(stack, PARAM_STACK_SIZE, PROT_NONE));
#endif
    }
  }

  SAFE_ASSERT(stack);
  return stack;
}

/**
 * Put a stack back into pool.
 * @param p The pool to put back into.
 * @param stack The stack to put back.
 */
void pool_put(void * stack)
{
  SAFE_ASSERT(stack);

  /** If local pool does not have space, */
  if (_pl.avail >= POOL_LOCAL_SIZE) {
    /** Try moving stacks to parent pool. */
    if (_pg.avail < POOL_GLOBAL_SIZE) {
      mutex_t mutex;
      mutex_lock(&_pg.lock, &mutex);

      /** Keep only POOL_CACHE_SIZE stacks. */
      while (_pl.avail > POOL_CACHE_SIZE && _pg.avail < POOL_GLOBAL_SIZE) {
        _pg.buff[_pg.avail++] = _pl.buff[--_pl.avail];
      }

      mutex_unlock(&_pg.lock, &mutex);
    }

    /** Free local pool for space. */
    while (_pl.avail >= POOL_LOCAL_SIZE) {
      free(_pl.buff[--_pl.avail]);
      STATS_DEC(N_STACKS, 1);
    }
  }

  /** Invariant: we always put stack into local pool. */
  _pl.buff[_pl.avail++] = stack;
}

