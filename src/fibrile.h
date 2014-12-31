#ifndef FIBRILE_H
#define FIBRILE_H

#include "fibrili.h"

/** fibril. */
#define fibril __attribute__((optimize("no-omit-frame-pointer")))

/** fibril_t. */
typedef struct _fibril_t fibril_t;

/** fibril_init. */
__attribute__((always_inline)) extern inline
void fibril_init(fibril_t * frptr)
{
  register void * rbp asm ("rbp");
  register void * rsp asm ("rsp");

  frptr->lock = 0;
  frptr->count = -1;
  frptr->stack.btm = rbp;
  frptr->stack.top = rsp;
}

/** fibril_join. */
__attribute__((always_inline)) extern inline
void fibril_join(fibril_t * frptr)
{
  if (frptr->count > -1 && !fibrili_join(frptr)) {
    fibrili_membar(fibrili_yield(frptr));
  }
}

#include "fork.h"

/** _fibril_fork_nrt. */
#define fibril_fork_nrt(fp, fn, ag) do { \
  __attribute__((noinline, hot, optimize(3))) \
  void _fibril_##fn##_fork(_fibril_defs ag fibril_t * f) { \
    fibrili_push(f); \
    fn(_fibril_args ag); \
    if (!fibrili_pop()) fibrili_resume(f); \
  } \
  fibrili_membar(_fibril_##fn##_fork(_fibril_expand ag fp)); \
} while (0)

/** _fibril_fork_wrt. */
#define fibril_fork_wrt(fp, rt, fn, ag) do { \
  __attribute__((noinline, hot, optimize(3))) \
  void _fibril_##fn##_fork(_fibril_defs ag fibril_t * f, __typeof__(&rt) p) { \
    fibrili_push(f); \
    *p = fn(_fibril_args ag); \
    if (!fibrili_pop()) fibrili_resume(f); \
  } \
  fibrili_membar(_fibril_##fn##_fork(_fibril_expand ag fp, &rt)); \
} while (0)

extern int fibril_rt_init(int nprocs);
extern int fibril_rt_exit();
extern int fibril_rt_nprocs(int nprocs);

#endif /* end of include guard: FIBRILE_H */
