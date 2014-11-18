#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "tlmap.h"

#define DEBUG_TID TID

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#ifndef DEBUG_WAIT
#define DEBUG_WAIT 0
#endif

#define DEBUG_CRIT 1
#define DEBUG_INFO 2
#define DEBUG_STEP 3

#define DEBUG_CONCAT(left, right) left##right
#define DEBUG_NARG(...) DEBUG_NARG_(__VA_ARGS__, ## __VA_ARGS__, \
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 0)
#define DEBUG_NARG_(...) DEBUG_ARG_N(__VA_ARGS__)
#define DEBUG_ARG_N(_1, _11, _2, _22, _3, _33, _4, _44, _5, _55, \
    _6, _66, _7, _77, _8, _88, N, ...) N

#define DEBUG_FORMAT(...) DEBUG_FORMAT_(DEBUG_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define DEBUG_FORMAT_(N, ...) DEBUG_CONCAT(DEBUG_FORMAT_, N)(__VA_ARGS__)
#define DEBUG_FORMAT_0(...)
#define DEBUG_FORMAT_1(p, ...) DEBUG_FORM p
#define DEBUG_FORMAT_2(p, ...) DEBUG_FORM p DEBUG_FORMAT_1(__VA_ARGS__)
#define DEBUG_FORMAT_3(p, ...) DEBUG_FORM p DEBUG_FORMAT_2(__VA_ARGS__)
#define DEBUG_FORMAT_4(p, ...) DEBUG_FORM p DEBUG_FORMAT_3(__VA_ARGS__)
#define DEBUG_FORMAT_5(p, ...) DEBUG_FORM p DEBUG_FORMAT_4(__VA_ARGS__)
#define DEBUG_FORMAT_6(p, ...) DEBUG_FORM p DEBUG_FORMAT_5(__VA_ARGS__)
#define DEBUG_FORMAT_7(p, ...) DEBUG_FORM p DEBUG_FORMAT_6(__VA_ARGS__)
#define DEBUG_FORM(var, spec) " " #var "=" spec

#define DEBUG_VARS(...) DEBUG_VARS_(DEBUG_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define DEBUG_VARS_(N, ...) DEBUG_CONCAT(DEBUG_VARS_, N)(__VA_ARGS__)
#define DEBUG_VARS_0(...)
#define DEBUG_VARS_1(p, ...) DEBUG_VAR p
#define DEBUG_VARS_2(p, ...) DEBUG_VAR p DEBUG_VARS_1(__VA_ARGS__)
#define DEBUG_VARS_3(p, ...) DEBUG_VAR p DEBUG_VARS_2(__VA_ARGS__)
#define DEBUG_VARS_4(p, ...) DEBUG_VAR p DEBUG_VARS_3(__VA_ARGS__)
#define DEBUG_VARS_5(p, ...) DEBUG_VAR p DEBUG_VARS_4(__VA_ARGS__)
#define DEBUG_VARS_6(p, ...) DEBUG_VAR p DEBUG_VARS_5(__VA_ARGS__)
#define DEBUG_VARS_7(p, ...) DEBUG_VAR p DEBUG_VARS_6(__VA_ARGS__)
#define DEBUG_VAR(var, spec) , var

#define DEBUG_DUMP(lv, tag, ...) do { \
  if (lv <= DEBUG_LEVEL) { \
    fprintf(stderr, "[%d]: " tag DEBUG_FORMAT(__VA_ARGS__) "\n", \
        DEBUG_TID DEBUG_VARS(__VA_ARGS__) \
    ); \
    fflush(stderr); \
  } \
} while (0)

#ifdef ENABLE_DEBUG

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG_ASSERT(F) do { \
  if (!(F)) { \
    DEBUG_DUMP(0, "assertion failed: " # F); \
    DEBUG_BREAK(!(F)); \
  } \
} while (0)

#define DEBUG_BREAK(T) do { \
  volatile int wait = (T); \
  if (wait) { \
    if (DEBUG_WAIT) { \
      int pid = getpid(); \
      DEBUG_DUMP(0, "waiting for debugger:", (pid, "%d")); \
    } else { \
      abort(); \
    } \
  } \
  while (wait); \
} while (0)

#else /* ENABLE_DEBUG is undefined */

#define DEBUG_ASSERT(...)
#define DEBUG_BREAK(...)

#endif /* end of ENABLE_DEBUG */
#endif /* end of include guard: DEBUG_H */
