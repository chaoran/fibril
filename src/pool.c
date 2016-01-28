#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"

#ifndef POOL_GLOBAL_SIZE
#define POOL_GLOBAL_SIZE 2048
#endif

#ifndef POOL_LOCAL_SIZE
#define POOL_LOCAL_SIZE 2
#endif

#ifndef POOL_CACHE_SIZE
#define POOL_CACHE_SIZE 1
#endif

struct {
  mutex_t * volatile lock;
  size_t volatile avail;
  long volatile total;
  void * buff[POOL_GLOBAL_SIZE];
} POOL;

typedef struct {
  size_t avail;
  void * buff[POOL_LOCAL_SIZE];
} pool_t;

static __thread pool_t * _pool;

static void * allocate(int force)
{
  void * stack = NULL;

  if (force || sync_fadd(POOL.total, 1) < POOL_GLOBAL_SIZE) {
    const size_t align = PARAM_PAGE_SIZE;
    const size_t nbyte = PARAM_STACK_SIZE;
    SAFE_RZCALL(posix_memalign(&stack, align, nbyte));
  } else {
    sync_fadd(POOL.total, -1);
  }

  return stack;
}

static void deallocate(void * addr)
{
  sync_fadd(POOL.total, -1);
  free(addr);
}

void pool_init()
{
  pool_t * p;
  SAFE_RZCALL(posix_memalign((void **) &p, PARAM_PAGE_SIZE, sizeof(pool_t)));
  p->avail = 0;

  _pool = p;
}

/**
 * Take a stack from the pool or allocate from heap if the pool is empty.
 * @return Return a stack or NULL if the pool has reached its limit.
 */
void * pool_take(int force)
{
  void * stack = NULL;
  pool_t * p = _pool;

  /** Take a stack from the available stacks. */
  if (p->avail > 0) {
    stack = p->buff[--p->avail];
  } else {
    /** Take a stack from the parent pool. */
    if (POOL.avail > 0) {
      mutex_t mutex;
      mutex_lock(&POOL.lock, &mutex);

      if (POOL.avail > 0) {
        stack = POOL.buff[--POOL.avail];
      }

      mutex_unlock(&POOL.lock, &mutex);
    }

    if (!stack) stack = allocate(force);
  }

  return stack;
}

/**
 * Put a stack back into pool.
 * @param p The pool to put back into.
 * @param stack The stack to put back.
 */
void pool_put(void * stack)
{
  pool_t * p = _pool;

  /** Put stack back into local pool. */
  if (p->avail < POOL_LOCAL_SIZE) {
    p->buff[p->avail++] = stack;
    return;
  }

  SAFE_ASSERT(p->avail == POOL_LOCAL_SIZE);

  /** Try putting stacks to parent pool. */
  if (POOL.avail < POOL_GLOBAL_SIZE) {
    mutex_t mutex;
    mutex_lock(&POOL.lock, &mutex);

    /** If parent pool has space, put stack into parent pool. */
    if (POOL.avail < POOL_GLOBAL_SIZE) {
      POOL.buff[POOL.avail++] = stack;
      stack = NULL;
    }

    /** Since we have acquired the parent lock, move more stacks back. */
    while (p->avail > POOL_CACHE_SIZE && POOL.avail < POOL_GLOBAL_SIZE) {
      POOL.buff[POOL.avail++] = p->buff[--p->avail];
    }

    mutex_unlock(&POOL.lock, &mutex);
  }

  /** Put stack back to heap. */
  if (stack) deallocate(stack);
  /** Remove extra stacks from local pool. */
  while (p->avail > POOL_CACHE_SIZE) deallocate(p->buff[--p->avail]);
}

