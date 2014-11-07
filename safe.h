#ifndef SAFE_H
#define SAFE_H

#include "debug.h"

#ifndef DISABLE_SAFE

#define SAFE_STRINGIFY(x) #x
#define SAFE_TOSTRING(x) SAFE_STRINGIFY(x)
#define SAFE_AT __FILE__ ":" SAFE_TOSTRING(__LINE__) ": "

#define SAFE_CALL(err, call) do { \
  int T = ((intptr_t) err == (intptr_t) (call)); \
  if (T) { \
    DEBUG_DUMP(0, "error: " SAFE_AT "%m"); \
    DEBUG_BREAK(T); \
  } \
} while(0)

#else
#define SAFE_CALL(err, call) do { call; } while (0)
#endif

#define SAFE_NNCALL(call) SAFE_CALL(-1, call);
#define SAFE_NZCALL(call) SAFE_CALL(0, call);

#endif /* end of include guard: SAFE_H */
