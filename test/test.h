#ifndef TEST_H
#define TEST_H

extern void init();
extern void prep();
extern void test();
extern int verify();

extern int n;

#include <stdlib.h>
#include <fibril.h>

#ifdef BENCHMARK

#include <stdio.h>
#include <float.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

static void sort(float * a, int n)
{
  int i, sorted = 0;

  while (!sorted) {
    sorted = 1;

    for (i = 1; i < n; ++i) {
      if (a[i] < a[i - 1]) {
        float t = a[i];
        a[i] = a[i - 1];
        a[i - 1] = t;
        sorted = 0;
      }
    }
  }
}

size_t static inline time_elapsed(size_t val)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec - val;
}

static void bench(const char * name, int nprocs)
{
  static int iter = 10;
  float times[iter];

  printf("===========================================\n");
  printf("  Benchmark: %s\n", strrchr(name, '/') + 1);
  printf("  Input size: %d\n", n);
  printf("  Number of iterations: %d\n", iter);
  printf("  Number of processors: %d\n", nprocs);

  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  long rss = ru.ru_maxrss;
  long flt = ru.ru_minflt;

  int i;
  for (i = 0; i < iter; ++i) {
    prep();
    size_t usecs = time_elapsed(0);
    test();
    usecs = time_elapsed(usecs);
    times[i] = usecs / 1000000.0;
    printf("  #%d execution time: %f s\n", i, times[i]);
  }

  sort(times, iter);

  float p10 = times[1];
  float p90 = times[8];
  float med = times[5];

  getrusage(RUSAGE_SELF, &ru);
  rss = ru.ru_maxrss - rss;
  flt = ru.ru_minflt - flt;

  printf("  Execution time summary:\n");
  printf("    Median: %f s\n", med);
  printf("    10th %%: %f s\n", p10);
  printf("    90th %%: %f s\n", p90);
  printf("  Resources summary: \n");
  printf("    Max RSS: %ld (KB)\n", ru.ru_maxrss);
  printf("    Runtime RSS: %ld (KB)\n", rss);
  printf("    # of page faults: %ld\n", flt);
}

#endif

#include <stdlib.h>

int main(int argc, const char *argv[])
{
  if (argc > 1 && (argc = atoi((void *) argv[1])) > 0) {
    n = argc;
  }

  init();

  fibril_rt_init(0);
  int nprocs = fibril_rt_nprocs();

#ifdef BENCHMARK
  bench(argv[0], nprocs);
#else
  prep();
  test();
#endif

  fibril_rt_exit();

#ifdef BENCHMARK
  printf("  Statistics summary:\n");
  printf("    # of steals: %s\n", getenv("FIBRIL_N_STEALS"));
  printf("    # of suspensions: %s\n", getenv("FIBRIL_N_SUSPENSIONS"));
  printf("    # of stacks used: %s\n", getenv("FIBRIL_N_STACKS"));
  printf("===========================================\n");
#endif
  return verify();
}

#endif /* end of include guard: TEST_H */
