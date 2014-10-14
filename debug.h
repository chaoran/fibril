#ifndef DEBUG_H
#define DEBUG_H

#ifdef ENABLE_DEBUG

#include <stdio.h>
#include "tls.h"

#ifndef ENABLE_SAFE
#define ENABLE_SAFE
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#define DEBUG_PRINT(format, ...) do { \
  fprintf(stderr, "[%d]: " format, _tls.tid, ## __VA_ARGS__); \
  fflush(stderr); \
} while (0)

/** Enable DEBUG_PRINT_IMPORTANT if DEBUG_LEVEL > 0 */
#if DEBUG_LEVEL > 0
#define DEBUG_PRINT_IMPORTANT DEBUG_PRINT
#else
#define DEBUG_PRINT_IMPORTANT(...)
#endif

/** Enable DEBUG_PRINT_INFO if DEBUG_LEVEL > 1 */
#if DEBUG_LEVEL > 1
#define DEBUG_PRINT_INFO DEBUG_PRINT
#else
#define DEBUG_PRINT_INFO(...)
#endif

/** Enable DEBUG_PRINT_VERBOSE if DEBUG_LEVEL > 2 */
#if DEBUG_LEVEL > 2
#define DEBUG_PRINT_VERBOSE DEBUG_PRINT
#else
#define DEBUG_PRINT_VERBOSE(...)
#endif

#else /** ifndef ENABLE_DEBUG */

#define DEBUG_PRINT(...)
#define DEBUG_PRINT_IMPORTANT(...)
#define DEBUG_PRINT_INFO(...)
#define DEBUG_PRINT_VERBOSE(...)

#endif

#endif /* end of include guard: DEBUG_H */
