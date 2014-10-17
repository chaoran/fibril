#ifndef TLS_H
#define TLS_H

#include "conf.h"
#include "page.h"

#define FIBRILi(xxx) FIBRILi_##xxx

typedef struct FIBRILi(_deque_t) {
  int lock;
  void ** head;
  void ** tail;
  void *  join;
} FIBRILi(deque_t);

typedef struct {
  struct _x {
    FIBRILi(deque_t) deq;
    int tid;
    int pid;
  } x __attribute__((aligned (sizeof(void *))));
  void *  buff[
    (NUM_TLS_PAGES * PAGE_SIZE - sizeof(struct _x)) / sizeof(void *)
  ];
} FIBRILi(tls_t) __attribute__((aligned (PAGE_SIZE)));

extern FIBRILi(tls_t) FIBRILi(tls);
#define FIBRILi_TLS (FIBRILi(tls).x)
#define FIBRILi_TID (FIBRILi(tls).x.tid)
#define FIBRILi_PID (FIBRILi(tls).x.pid)
#define FIBRILi_DEQ (FIBRILi(tls).x.deq)

static inline void FIBRILi(push)()
{
  void * stptr;
  asm volatile ( "mov %%rsp, %0" : "=g" (stptr) : : ); \

  *FIBRILi_DEQ.tail++ = stptr;
}

#define FIBRILi_fence __sync_synchronize
#define FIBRILi_lock(ptr) while (__sync_lock_test_and_set(ptr, 1))
#define FIBRILi_unlock(ptr) __sync_lock_release(ptr)

static inline int FIBRILi(pop)()
{
  void ** tail = --FIBRILi_DEQ.tail;
  FIBRILi_fence();

  if (FIBRILi_DEQ.head > tail) {
    FIBRILi_DEQ.tail++;

    FIBRILi_lock(&FIBRILi_DEQ.lock);
    tail = --FIBRILi_DEQ.tail;

    if (FIBRILi_DEQ.head > tail) {
      FIBRILi_DEQ.tail++;
      FIBRILi_unlock(&FIBRILi_DEQ.lock);
      return 0;
    }

    FIBRILi_unlock(&FIBRILi_DEQ.lock);
  }

  return 1;
}

#endif /* end of include guard: TLS_H */
