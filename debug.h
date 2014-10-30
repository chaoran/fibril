#ifndef DEBUG_H
#define DEBUG_H

#ifdef ENABLE_DEBUG

#include <stdio.h>
//#include "tls.h"

#define DEBUG_TID fibrile_deq.tid
#define DEBUG_PID fibrile_deq.pid

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#define DEBUG_PRINT(format, ...) do { \
  fprintf(stderr, "[%d]: " format, DEBUG_TID, ## __VA_ARGS__); \
  fflush(stderr); \
} while (0)

/** Enable DEBUG_PRINTC if DEBUG_LEVEL > 0 */
#if DEBUG_LEVEL > 0
#define DEBUG_PRINTC DEBUG_PRINT
#else
#define DEBUG_PRINTC(...)
#endif

/** Enable DEBUG_PRINTI if DEBUG_LEVEL > 1 */
#if DEBUG_LEVEL > 1
#define DEBUG_PRINTI DEBUG_PRINT
#else
#define DEBUG_PRINTI(...)
#endif

/** Enable DEBUG_PRINTV if DEBUG_LEVEL > 2 */
#if DEBUG_LEVEL > 2
#define DEBUG_PRINTV DEBUG_PRINT
#else
#define DEBUG_PRINTV(...)
#endif

#else /** ifndef ENABLE_DEBUG */

#define DEBUG_PRINT(...)
#define DEBUG_PRINTC(...)
#define DEBUG_PRINTI(...)
#define DEBUG_PRINTV(...)

#endif

#endif /* end of include guard: DEBUG_H */
