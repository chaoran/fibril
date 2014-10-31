#ifndef UTIL_H
#define UTIL_H

#include "fibrile.h"

/** Atomics. */
#define fence() fibrile_fence()
#define fadd(ptr, n) __sync_fetch_and_add(ptr, n)
#define cas(...) __sync_val_compare_and_swap(__VA_ARGS__)

/** Lock. */
#define lock(ptr)   fibrile_lock(ptr)
#define unlock(ptr) fibrile_unlock(ptr)
#define trylock(ptr) (!__sync_lock_test_and_set(ptr, 1))

/** Barrier. */
static inline void barrier(int n)
{
  static volatile int count = 0;
  static volatile int sense = 0;

  int local_sense = !fibrile_deq.sense;

  if (fadd(&count, 1) == n - 1) {
    count = 0;
    sense = local_sense;
  }

  while (sense != local_sense);
}

/** Others. */

typedef struct _frame_t {
  void * rsp;
  void * rip;
} frame_t;

#define this_frame() ((frame_t *) __builtin_frame_address(0))
#define unreachable() __builtin_unreachable()

#define adjust(addr, offset) ((void *) addr - offset)

#endif /* end of include guard: UTIL_H */
