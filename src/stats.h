#ifndef STATS_H
#define STATS_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

//#define FIBRIL_STATS

#ifndef FIBRIL_STATS

#define STATS_COUNT(...)
#define STATS_INC(...)
#define STATS_DEC(...)
#define STATS_EXPORT(...)

#else // FIBRIL_STATS defined

#include <stdlib.h>
#include "sync.h"

typedef enum _stats_t {
  N_STEALS = 0,
  N_SUSPENSIONS,
  N_STACKS,
  N_PAGES,
  STATS_LAST_ENTRY /** No more enum entries after this. */
} stats_t;

extern struct _stats_counter_t {
  volatile long curr;
  volatile size_t peak;
} _stats_table[STATS_LAST_ENTRY];

#define STATS_COUNT(e, n) do { \
  sync_fadd(_stats_table[e].peak, n); \
} while (0)

#define STATS_INC(e, n) do { \
  long curr = sync_fadd(_stats_table[e].curr, n); \
  while (1) { \
    size_t peak = _stats_table[e].peak; \
    if (peak > curr) break; \
    if (sync_cas(&_stats_table[e].peak, peak, curr + 1)) break; \
  } \
} while (0)

#define STATS_DEC(e, n) do { \
  sync_fadd(_stats_table[e].curr, -n); \
} while (0)

#define STATS_EXPORT(e) do { \
  char tmp[32]; \
  sprintf(tmp, "%ld", _stats_table[e].peak); \
  setenv("FIBRIL_" #e, tmp, 1); \
} while (0)

#endif
#endif /* end of include guard: STATS_H */
