#ifndef UTIL_H
#define UTIL_H

#include "tls.h"
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
  static volatile int __fibril_local__ _local_sense;

  int sense = !_local_sense;

  if (fadd(&_count, 1) == _nprocs - 1) {
    _count = 0;
    _sense = sense;
  }

  while (_sense != sense);
  _local_sense = sense;
  fence();
}

/** Others. */
#define unreachable() __builtin_unreachable()
#define adjust(addr, offset) ((void *) addr - offset)

#endif /* end of include guard: UTIL_H */
