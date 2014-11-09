#ifndef FIBRIL_H
#define FIBRIL_H

#include "fibrile.h"

typedef struct _fibril_t fibril_t;

extern int FIBRIL_NPROCS;

extern inline __attribute__ ((always_inline))
void fibril_make(fibril_t * frptr)
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

extern inline __attribute__ ((always_inline))
void fibril_join(fibril_t * frptr)
{
  if ((frptr)->jtp && !fibrile_join(frptr)) {
    fibrile_save(frptr, &&AFTER_JOIN);
    fibrile_yield(frptr);
  }
AFTER_JOIN: return;
}

#define FIBRIL_SUCCESS 0

#endif /* end of include guard: FIBRIL_H */
