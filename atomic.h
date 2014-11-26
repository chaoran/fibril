#ifndef ATOMIC_H
#define ATOMIC_H

#include "fibrili.h"

#define atomic_fence() fibrili_fence()
#define atomic_lock(lock) fibrili_lock(lock)
#define atomic_unlock(lock) fibrili_unlock(lock)

#endif /* end of include guard: ATOMIC_H */
