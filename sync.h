#ifndef SYNC_H
#define SYNC_H

#include "synce.h"

/** Atomics. */
#define sync_fence FIBRILe_fence
#define sync_fadd  __sync_fetch_and_add
#define sync_addf  __sync_add_and_fetch
#define sync_cas   __sync_val_compare_and_swap

/** Lock. */
#define sync_lock   FIBRILe_lock
#define sync_unlock FIBRILe_unlock

#define sync_barrier(ptr, n) do { \
  int v = sync_addf(ptr, 1); \
  while (v <= n && (v = sync_cas(ptr, n, 0)) && v != n); \
} while (0)

#endif /* end of include guard: SYNC_H */
