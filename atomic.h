#ifndef ATOMIC_H
#define ATOMIC_H

#include "fibrili.h"

#define atomic_fence() fibrili_fence()
#define atomic_lock(lock) fibrili_lock(lock)
#define atomic_unlock(lock) fibrili_unlock(lock)
#define atomic_load(val) __atomic_load_n(&(val), __ATOMIC_ACQUIRE)
#define atomic_fadd(val, n) __atomic_fetch_add(&(val), n, __ATOMIC_ACQ_REL)

static inline void atomic_barrier(int nprocs)
{
  static volatile int _count;
  static volatile int _sense;
  static volatile __thread int _local_sense;

  int sense = !_local_sense;

  if (atomic_fadd(_count, 1) == nprocs - 1) {
    _count = 0;
    _sense = sense;
  }

  while (_sense != sense);
  _local_sense = sense;
  atomic_fence();
}

#endif /* end of include guard: ATOMIC_H */
