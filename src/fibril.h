#ifndef FIBRIL_H
#define FIBRIL_H

typedef struct _fibril_t fibril_t;

#define FIBRIL_SUCCESS 0

/**
 * These are special arguments to fibril_rt_init().
 * FIBRIL_NPROCS tells the runtime to fetch the number of processors
 * from the environment variable FIBRIL_NPROCS (getenv(FIBRIL_NPROCS)).
 * FIBRIL_NPROCS_ONLN tells the runtime to use all available processors
 * in the system (sysconf(_SC_NPROCESSORS_ONLN)).
 */
#define FIBRIL_NPROCS 0
#define FIBRIL_NPROCS_ONLN -1

/**
 * Compile with -DFIBRIL_SERIAL to get a serial version of a fibril program.
 */
#ifndef FIBRIL_SERIAL

#define fibril __attribute__((optimize("-fno-omit-frame-pointer")))

#include <fibril/fork.h>
#define fibril_fork fibrili_fork

#define fibril_init(frptr) do { \
  fibril_t * f = (frptr); \
  register void * rsp asm ("rsp"); \
  f->lock = 0; \
  f->count = -1; \
  f->stack.top = rsp; \
} while (0)

#include <fibril/fibrili.h>

#define fibril_join(frptr) do { \
  fibril_t * f = (frptr); \
  if (f->count == -1 || fibrili_join(f)) break; \
  if (fibrili_setjmp(&f->state)) break; \
  fibrili_yield(f); \
} while (0)

extern int fibril_rt_init(int nprocs);
extern int fibril_rt_exit();

#else /* #ifdef FIBRIL_SERIAL */

#define fibril

#define fibril_fork(...)

#define fibril_init(frptr)
#define fibril_join(frptr)

#define fibril_rt_init(nprocs)
#define fibril_rt_exit()

#endif /* end of #ifndef FIBRIL_SERIAL */

#endif /* end of include guard: FIBRIL_H */
