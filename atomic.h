#ifndef ATOMIC_H
#define ATOMIC_H

#define atomic_fadd __sync_fetch_and_add
#define atomic_fence __sync_synchronize

#endif /* end of include guard: ATOMIC_H */
