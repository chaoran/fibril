#ifndef SYNC_H
#define SYNC_H

/** Atomics. */
#define sync_fadd  __sync_fetch_and_add
#define sync_fence __sync_synchronize
#define sync_cas   __sync_val_compare_and_swap

/** Lock. */
#define sync_lock(ptr) while (__sync_lock_test_and_set(ptr, 1))
#define sync_unlock(ptr) __sync_lock_release(ptr)

#endif /* end of include guard: SYNC_H */
