#ifndef SYNCE_H
#define SYNCE_H

#define FIBRILe_fence() __sync_synchronize()
#define FIBRILe_lock(ptr) while (__sync_lock_test_and_set(ptr, 1))
#define FIBRILe_unlock(ptr) __sync_lock_release(ptr)

#endif /* end of include guard: SYNCE_H */
