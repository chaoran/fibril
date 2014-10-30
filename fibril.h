#ifndef FIBRIL_H
#define FIBRIL_H

typedef struct _fibril_t {
  void * jtp;
  void * rsp;
  void * rbp;
  void * rbx;
  void * r12;
  void * r13;
  void * r14;
  void * r15;
  void * rip;
} fibril_t;

#include "fibrile.h"

extern int fibril_init(int nprocs);
extern int fibril_exit();

extern inline void __attribute__ ((always_inline))
fibril_new(fibril_t * frptr)
{
  register void * rsp asm ("rsp");

  frptr->jtp = NULL;
  frptr->rsp = rsp;
}

#define FIBRIL_FORK(frptr, fn, retval, ...) do { \
  fibrile_save(frptr, &&AFTER_FORK_##fn##__FILE__##__LINE__); \
  fibrile_push(frptr); \
  retval = fn(__VA_ARGS__); \
  if (fibrile_pop()) break; \
  fibrile_data_t data[1] = { { &retval, sizeof(retval) } }; \
  fibrile_join(frptr, data, 1); \
  AFTER_FORK_##fn##__FILE__##__LINE__: break; \
} while (0)

extern inline void __attribute__ ((always_inline))
fibril_join(fibril_t * frptr)
{
  if ((frptr)->jtp) _fibril_join(frptr);
}

#define FIBRIL_SUCCESS 0

#endif /* end of include guard: FIBRIL_H */
