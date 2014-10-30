#ifndef SAFE_H
#define SAFE_H

#ifdef ENABLE_SAFE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "debug.h"

#ifdef ENABLE_DEBUG
#define SAFE_ABORT(cond) do { \
  volatile int bp = !(cond); \
  if (bp) DEBUG_PRINT("process %d is waiting for debugger\n", DEBUG_PID); \
  while (bp); \
} while (0)
#else
#define SAFE_ABORT(cond) abort()
#endif

#define SAFE_ASSERT(cond) do { \
  if (!(cond)) { \
    DEBUG_PRINT("assertion failed: " # cond "\n"); \
    SAFE_ABORT(cond); \
  } \
} while (0)

#define SAFE_RETURN(ret, call) do { \
  if (-1 == (intptr_t) (ret = (call))) { \
    perror(#call); \
    SAFE_ABORT(1); \
  } \
} while(0)

#define SAFE_FNCALL(call) do { \
  if (-1 == (intptr_t) (call)) { \
    perror(#call); \
    SAFE_ABORT(1); \
  } \
} while(0)

#else

#define SAFE_ASSERT(...)
#define SAFE_RETURN(ret, call) (ret = (call))
#define SAFE_FNCALL(call) (call)

#endif

#endif /* end of include guard: SAFE_H */
