#ifndef UTIL_H
#define UTIL_H

#include "tlmap.h"
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
static inline void barrier()
{
  extern int _nprocs;
  static volatile int _count;
  static volatile int _sense;
  static __fibril_local__ int local;

  int sense = !local;
  int n = _nprocs - 1;

  if (fadd(&_count, 1) == n) {
    _count = 0;
    _sense = sense;
  }

  while (_sense != sense);
  local = sense;
  fence();
}

/** Others. */
#define unreachable() __builtin_unreachable()
#define adjust(addr, offset) ((void *) addr - offset)

#endif /* end of include guard: UTIL_H */
