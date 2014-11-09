#ifndef SAFE_H
#define SAFE_H

#include <errno.h>
#include <string.h>
#include "debug.h"

#define SAFE_PRINT_BUFF_SIZE 1024

#ifndef DISABLE_SAFE

#define SAFE_CALL(err, call) do { \
  int T = ((intptr_t) err == (intptr_t) (call)); \
  if (T) { \
    char buff[SAFE_PRINT_BUFF_SIZE]; \
    snprintf(buff, SAFE_PRINT_BUFF_SIZE, "ERROR: %s", strerror(errno)); \
    DEBUG_DUMP(0, buff); \
    DEBUG_BREAK(T); \
  } \
} while(0)

#define SAFE_ASSERT(F) do { \
  if (!(F)) { \
    DEBUG_DUMP(0, "assertion failed: " # F "\n"); \
    DEBUG_BREAK(!(F)); \
  } \
} while (0)

#else
#define SAFE_CALL(err, call) do { call; } while (0)
#define SAFE_ASSERT(...)
#endif

#define SAFE_NNCALL(call) SAFE_CALL(-1, call);
#define SAFE_NZCALL(call) SAFE_CALL(0, call);

#define SAFE_WARN(msg, ...) do { \
  char buff[SAFE_PRINT_BUFF_SIZE]; \
  snprintf(buff, SAFE_PRINT_BUFF_SIZE, "WARNING: " msg, ## __VA_ARGS__); \
  DEBUG_DUMP(0, buff); \
} while (0)

#endif /* end of include guard: SAFE_H */
