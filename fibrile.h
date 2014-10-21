#ifndef FIBRILE_H
#define FIBRILE_H

#include "conf.h"

#define FIBRILe(xxx) FIBRILe_##xxx

/** Types. */
typedef struct FIBRILe(_deque_t) {
  int lock;
  void ** head;
  void ** tail;
  void *  join;
} FIBRILe(deque_t);

typedef struct FIBRILe(_tls_t) {
  struct _x {
    FIBRILe(deque_t) deq;
    int tid;
    int pid;
    struct FIBRILe(_tls_t) ** tls;
  } x __attribute__((aligned (sizeof(void *))));
  void *  buff[
    (NUM_TLS_PAGES * PAGE_SIZE - sizeof(struct _x)) / sizeof(void *)
  ];
} FIBRILe(tls_t) __attribute__((aligned (PAGE_SIZE)));

/** Variables. */
extern FIBRILe(tls_t) FIBRILe(tls);
#define FIBRILe_TLS (FIBRILe(tls))
#define FIBRILe_TID (FIBRILe(tls).x.tid)
#define FIBRILe_PID (FIBRILe(tls).x.pid)
#define FIBRILe_DEQ (FIBRILe(tls).x.deq)

/** Macros. */
#include "synce.h"

static inline void FIBRILe(push)()
{
  void * stptr;
  asm volatile ( "mov %%rsp, %0" : "=g" (stptr) : : ); \

  *FIBRILe_DEQ.tail++ = stptr;
}

static inline int FIBRILe(pop)()
{
  void ** tail = --FIBRILe_DEQ.tail;
  FIBRILe_fence();

  if (FIBRILe_DEQ.head > tail) {
    FIBRILe_DEQ.tail++;

    FIBRILe_lock(&FIBRILe_DEQ.lock);
    tail = --FIBRILe_DEQ.tail;

    if (FIBRILe_DEQ.head > tail) {
      FIBRILe_DEQ.tail++;
      FIBRILe_unlock(&FIBRILe_DEQ.lock);
      return 0;
    }

    FIBRILe_unlock(&FIBRILe_DEQ.lock);
  }

  return 1;
}

/** Macros to count the number of variadic macro arguments. */
#define FIBRILe_CONCAT(left, right) left##right
#define FIBRILe_NARG(...) FIBRILe_NARG_(__VA_ARGS__, ##__VA_ARGS__, \
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 0)
#define FIBRILe_NARG_(...) FIBRILe_ARG_N(__VA_ARGS__)
#define FIBRILe_ARG_N(_1, _11, _2, _22, _3, _33, _4, _44, _5, _55, \
    _6, _66, _7, _77, _8, _88, N, ...) N

/** FIBRILe_SAVE */
#define FIBRILe_SAVE(...) \
  FIBRILe_SAVE_(FIBRILe_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define FIBRILe_SAVE_(N, ...) \
  FIBRILe_CONCAT(FIBRILe_SAVE_, N)(__VA_ARGS__)
#define FIBRILe_SAVE_d(type, var) type volatile _fr_##var
#define FIBRILe_SAVE_v(type, var) var
#define FIBRILe_SAVE_s(var) FIBRILe_SAVE_d var = FIBRILe_SAVE_v var;
#define FIBRILe_SAVE_0(...)
#define FIBRILe_SAVE_1(var, ...) FIBRILe_SAVE_s(var)
#define FIBRILe_SAVE_2(var, ...) FIBRILe_SAVE_s(var) FIBRILe_SAVE_1(__VA_ARGS__)
#define FIBRILe_SAVE_3(var, ...) FIBRILe_SAVE_s(var) FIBRILe_SAVE_2(__VA_ARGS__)
#define FIBRILe_SAVE_4(var, ...) FIBRILe_SAVE_s(var) FIBRILe_SAVE_3(__VA_ARGS__)
#define FIBRILe_SAVE_5(var, ...) FIBRILe_SAVE_s(var) FIBRILe_SAVE_4(__VA_ARGS__)
#define FIBRILe_SAVE_6(var, ...) FIBRILe_SAVE_s(var) FIBRILe_SAVE_5(__VA_ARGS__)
#define FIBRILe_SAVE_7(var, ...) FIBRILe_SAVE_s(var) FIBRILe_SAVE_6(__VA_ARGS__)

/** FIBRILe_REST */
#define FIBRILe_REST(...) \
  FIBRILe_REST_(FIBRILe_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define FIBRILe_REST_(N, ...) \
  FIBRILe_CONCAT(FIBRILe_REST_, N)(__VA_ARGS__)
#define FIBRILe_REST_a(type, var) var = _fr_##var
#define FIBRILe_REST_s(var) FIBRILe_REST_a var;
#define FIBRILe_REST_0(...)
#define FIBRILe_REST_1(var, ...) FIBRILe_REST_s(var)
#define FIBRILe_REST_2(var, ...) FIBRILe_REST_s(var) FIBRILe_REST_1(__VA_ARGS__)
#define FIBRILe_REST_3(var, ...) FIBRILe_REST_s(var) FIBRILe_REST_2(__VA_ARGS__)
#define FIBRILe_REST_4(var, ...) FIBRILe_REST_s(var) FIBRILe_REST_3(__VA_ARGS__)
#define FIBRILe_REST_5(var, ...) FIBRILe_REST_s(var) FIBRILe_REST_4(__VA_ARGS__)
#define FIBRILe_REST_6(var, ...) FIBRILe_REST_s(var) FIBRILe_REST_5(__VA_ARGS__)
#define FIBRILe_REST_7(var, ...) FIBRILe_REST_s(var) FIBRILe_REST_6(__VA_ARGS__)

#endif /* end of include guard: FIBRILE_H */
