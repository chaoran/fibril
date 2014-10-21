#ifndef SAFE_H
#define SAFE_H

#ifdef ENABLE_SAFE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "fibrile.h"

#define SAFE_TID FIBRILe_TID

#define SAFE_ASSERT(cond) do { \
  if (!(cond)) { \
    fprintf(stderr, "[%d]: assertion failed: " # cond "\n", SAFE_TID); \
    fflush(stderr); \
    abort(); \
  } \
} while (0)

#define SAFE_RETURN(ret, call) do { \
  if (-1 == (intptr_t) (ret = (call))) { \
    perror(#call); \
    abort(); \
  } \
} while(0)

#define SAFE_FNCALL(call) do { \
  if (-1 == (intptr_t) (call)) { \
    perror(#call); \
    abort(); \
  } \
} while(0)

#else

#define SAFE_ASSERT(...)
#define SAFE_RETURN(ret, call) (ret = (call))
#define SAFE_FNCALL(call) (call)

#endif

#endif /* end of include guard: SAFE_H */
