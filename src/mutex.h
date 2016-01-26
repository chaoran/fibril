#ifndef MUTEX_H
#define MUTEX_H

#define MUTEX_LOCKED 1

typedef struct _mutex_t {
  struct _mutex_t * volatile next;
  volatile char flag;
} mutex_t __attribute__((aligned(128)));

void mutex_lock   (mutex_t ** mutex, mutex_t * node);
int  mutex_trylock(mutex_t ** mutex, mutex_t * node);
void mutex_unlock (mutex_t ** mutex, mutex_t * node);

#endif /* end of include guard: MUTEX_H */
