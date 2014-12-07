#ifndef SAFE_H
#define SAFE_H

#include "debug.h"

#ifndef DISABLE_SAFE

#define SAFE_STRINGIFY(x) #x
#define SAFE_TOSTRING(x) SAFE_STRINGIFY(x)
#define SAFE_AT __FILE__ ":" SAFE_TOSTRING(__LINE__) ": "

#define SAFE_ASSERT(cond) do { \
  if (!(cond)) { \
    DEBUG_DUMP(0, "error: " SAFE_AT "%m"); \
    DEBUG_BREAK(!(cond)); \
  } \
} while (0)

#else
#define SAFE_ASSERT(...)
#endif

#include <stdint.h>

#define SAFE_NNCALL(call) do { \
  intptr_t ret = (intptr_t) (call); \
  SAFE_ASSERT(ret >= 0); \
} while (0)

#define SAFE_NZCALL(call) do { \
  intptr_t ret = (intptr_t) (call); \
  SAFE_ASSERT(ret != 0); \
} while (0)

#define SAFE_RZCALL(call) do { \
  intptr_t ret = (intptr_t) (call); \
  SAFE_ASSERT(ret == 0); \
} while (0)

#endif /* end of include guard: SAFE_H */
