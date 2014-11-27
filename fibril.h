#ifndef FIBRIL_H
#define FIBRIL_H

#include "fibrili.h"

typedef struct _fibril_t fibril_t;

#define FIBRIL_SUCCESS 0
#define FIBRIL_MAX_PROCS -1

#define fibril_init(frptr) do { \
  fibril_t * f = (frptr); \
  f->count = -1; \
  f->lock  = 0; \
} while (0)

#define fibril_fork(frptr, fn, ret, ...) do { \
  __label__ AFTER_FORK; \
  fibril_t * f = (frptr); \
  fibrili_save(f); \
  f->regs.rip = &&AFTER_FORK; \
  fibrili_push(f); \
  ret = fn(__VA_ARGS__); \
  if (!fibrili_pop()) { \
    volatile __auto_type retptr = &ret; \
    *retptr = ret; \
    fibrili_resume(f); \
  } \
  AFTER_FORK: fibrili_flush(); \
} while (0)

#define fibril_join(frptr) do { \
  fibril_t * f = (frptr); \
  if (f->count == -1 || fibrili_join(f)) break; \
  fibrili_save(f); \
  fibrili_yield(f); \
} while (0)

extern int fibril_rt_init(int nprocs);
extern int fibril_rt_exit();

#endif /* end of include guard: FIBRIL_H */
