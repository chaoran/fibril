#ifndef STATS_H
#define STATS_H

#include <stddef.h>
#include <atomic.h>

#ifndef STATS_MAX_COUNTERS
#define STATS_MAX_COUNTERS 32
#endif

#define ENABLE_STATS_STACK_PAGES

#ifdef ENABLE_STATS_STACK_PAGES
#define STATS_STACK_PAGES 1

#ifndef ENABLE_STATS
#define ENABLE_STATS
#endif

#else
#define STATS_STACK_PAGES 0
#endif

#ifdef ENABLE_STATS

typedef struct _stats_t {
  size_t cur __attribute__((aligned(64)));
  size_t max __attribute__((aligned(64)));
} stats_t;

extern stats_t STATS_COUNTERS[STATS_MAX_COUNTERS];

#define STATS_COUNTER_INC(id, delta) do { \
  if (id > 0) { \
    size_t n = atomic_addf(&STATS_COUNTERS[id].cur, delta); \
    size_t m = STATS_COUNTERS[id].max; \
    while (n > m) m = atomic_cas(&STATS_COUNTERS[id].max, m, n); \
  } \
} while (0)

#define STATS_COUNTER_DEC(id, delta) do { \
  if (id > 0) { \
    atomic_addf(&STATS_COUNTERS[id].cur, -(delta)); \
  } \
} while (0)

#define STATS_COUNTER_GET_MAX(id) (STATS_COUNTERS[id].max)

#endif
#endif /* end of include guard: STATS_H */
