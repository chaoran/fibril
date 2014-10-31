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

#define SAFE_STRINGIFY(x) #x
#define SAFE_TOSTRING(x) SAFE_STRINGIFY(x)
#define SAFE_AT __FILE__ ":" SAFE_TOSTRING(__LINE__) ": "

#define SAFE_ASSERT(cond) do { \
  if (!(cond)) { \
    DEBUG_PRINT("assertion failed: " SAFE_AT # cond "\n"); \
    SAFE_ABORT(cond); \
  } \
} while (0)

#define mkstr1(X) #X
#define mkstr2(X) mkstr1(X)

#define SAFE_RETURN(ret, call) do { \
  if (-1 == (intptr_t) (ret = (call))) { \
    perror("error: " SAFE_AT #call); \
    SAFE_ABORT(0); \
  } \
} while(0)

#define SAFE_FNCALL(call) do { \
  if (-1 == (intptr_t) (call)) { \
    perror("error: " SAFE_AT #call); \
    SAFE_ABORT(0); \
  } \
} while(0)

#else

#define SAFE_ASSERT(...)
#define SAFE_RETURN(ret, call) do { ret = call; } while (0)
#define SAFE_FNCALL(call) do { call; } while (0)

#endif

#endif /* end of include guard: SAFE_H */
