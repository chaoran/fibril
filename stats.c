#ifdef ENABLE_STATS

#include <stdio.h>
#include <sys/resource.h>

static size_t stats_peek_rss()
{
  struct rusage rusage;
  getrusage( RUSAGE_SELF, &rusage );
  return (size_t) (rusage.ru_maxrss * 1024L);
}

__attribute__((destructor))
void _print_stats(void)
{
  size_t rss = stats_peek_rss();
  printf("Maximum RSS: %ld KB\n", rss / 1024L);
}

#endif
