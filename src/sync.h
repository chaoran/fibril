#ifndef SYNC_H
#define SYNC_H

#include "fibrili.h"

#define sync_fence() fibrili_fence()
#define sync_lock(lock) fibrili_lock(lock)
#define sync_unlock(lock) fibrili_unlock(lock)

#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ > 7

#define sync_fadd(val, n) __atomic_fetch_add(&(val), n, __ATOMIC_ACQ_REL)
#define sync_cas(ptr, cmp, val) __sync_val_compare_and_swap(ptr, cmp, val)
#define sync_swap(ptr, val) __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL)

#else
#if defined(__x86_64__) || defined(_M_X64_)

#define sync_fadd(val, n) __sync_fetch_and_add(&(val), n)
#define sync_cas(ptr, cmp, val) __sync_val_compare_and_swap(ptr, cmp, val)
#define sync_swap(ptr, val) __sync_lock_test_and_set(ptr, val)

#endif
#endif

static inline void sync_barrier(int nprocs)
{
  static volatile int _count;
  static volatile int _sense;
  static __thread volatile int _local_sense;

  int sense = !_local_sense;

  if (sync_fadd(_count, 1) == nprocs - 1) {
    _count = 0;
    _sense = sense;
  }

  while (_sense != sense);
  _local_sense = sense;
  sync_fence();
}

#endif /* end of include guard: SYNC_H */
