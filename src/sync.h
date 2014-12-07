#ifndef SYNC_H
#define SYNC_H

#include "fibrili.h"

#define sync_fence() fibrili_fence()
#define sync_lock(lock) fibrili_lock(lock)
#define sync_unlock(lock) fibrili_unlock(lock)
#define sync_load(val) __atomic_load_n(&(val), __ATOMIC_ACQUIRE)
#define sync_fadd(val, n) __atomic_fetch_add(&(val), n, __ATOMIC_ACQ_REL)

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
