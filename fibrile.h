#ifndef FIBRILE_H
#define FIBRILE_H

#include <stddef.h>

struct _fibril_t {
  void * jtp;
  void * rsp;
  void * rbp;
  void * rbx;
  void * r12;
  void * r13;
  void * r14;
  void * r15;
  void * rip;
};

typedef struct {
  void * addr;
  size_t size;
} fibrile_data_t;

extern int fibrile_join(const struct _fibril_t * frptr);

extern __attribute__ ((noreturn)) void
fibrile_yield(const struct _fibril_t * frptr);

extern __attribute__ ((noreturn)) void
fibrile_resume(const struct _fibril_t *, const fibrile_data_t *, size_t);

extern inline void __attribute__ ((always_inline))
fibrile_save(struct _fibril_t * frptr, void * rip)
{
  __asm__ (
      "mov\t%%rbp,%0\n\t"
      "mov\t%%rbx,%1\n\t"
      "mov\t%%r12,%2\n\t"
      "mov\t%%r13,%3\n\t"
      "mov\t%%r14,%4\n\t"
      "mov\t%%r15,%5\n\t"
      "movq\t%7,%6" :
      "=m" (frptr->rbp), "=m" (frptr->rbx),
      "=m" (frptr->r12), "=m" (frptr->r13),
      "=m" (frptr->r14), "=m" (frptr->r15),
      "=m" (frptr->rip) : "ir" (rip)
  );
}

#define FIBRILE_PAGE_SIZE (4096ULL)
#define FIBRILE_TLS_SIZE  (2 * FIBRILE_PAGE_SIZE)
#define FIBRILE_PTR_SIZE  (sizeof(void *))

typedef struct {
  struct _fibrile_deque_t {
    int tid;
    int pid;
    int lock;
    int head;
    int tail;
    void * base;
    struct _fibrile_deque_t **  deqs;
  } deq __attribute__ ((aligned (FIBRILE_PTR_SIZE)));
  struct _fibril_t * buff[
    (FIBRILE_TLS_SIZE - sizeof(struct _fibrile_deque_t)) / FIBRILE_PTR_SIZE
  ];
} fibrile_tls_t __attribute__((aligned (FIBRILE_PAGE_SIZE)));

extern fibrile_tls_t fibrile_tls;
#define fibrile_deq (fibrile_tls.deq)

#include "debug.h"

static inline void fibrile_push(struct _fibril_t * frptr)
{
  fibrile_tls.buff[fibrile_tls.deq.tail++] = frptr;
  DEBUG_PRINTV("push: frptr=%p rsp=%p rip=%p\n", frptr, frptr->rsp, frptr->rip);
}

#define fibrile_fence() __sync_synchronize()
#define fibrile_lock(ptr) while (__sync_lock_test_and_set(ptr, 1))
#define fibrile_unlock(ptr) __sync_lock_release(ptr)

static inline int fibrile_pop()
{
  int T = fibrile_deq.tail;

  if (T == 0) return 0;

  fibrile_deq.tail = --T;

  fibrile_fence();
  int H = fibrile_deq.head;

  if (H > T) {
    fibrile_deq.tail = T + 1;

    fibrile_lock(&fibrile_deq.lock);
    H = fibrile_deq.head;

    if (H > T) {
      fibrile_deq.head = 0;
      fibrile_deq.tail = 0;

      fibrile_unlock(&fibrile_deq.lock);
      return 0;
    }

    fibrile_deq.tail = T;
    fibrile_unlock(&fibrile_deq.lock);
  }

  return 1;
}

/** Macros to count the number of variadic macro arguments. */
#define FIBRILE_CONCAT(left, right) left##right
#define FIBRILE_NARG(...) FIBRILE_NARG_(__VA_ARGS__, ##__VA_ARGS__, \
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 0)
#define FIBRILE_NARG_(...) FIBRILE_ARG_N(__VA_ARGS__)
#define FIBRILE_ARG_N(_1, _11, _2, _22, _3, _33, _4, _44, _5, _55, \
    _6, _66, _7, _77, _8, _88, N, ...) N

#endif /* end of include guard: FIBRILE_H */
