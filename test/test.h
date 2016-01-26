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

  printf("  Execution time summary:\n");
  printf("    Median: %f s\n", med);
  printf("    10th %%: %f s\n", p10);
  printf("    90th %%: %f s\n", p90);
  printf("===========================================\n");
}

#endif

#include <stdlib.h>

int main(int argc, const char *argv[])
{
  if (argc > 1 && (argc = atoi((void *) argv[1])) > 0) {
    n = argc;
  }

  init();

  int nprocs = fibril_rt_nprocs(0);
  fibril_rt_init(nprocs);

#ifdef BENCHMARK
  bench(argv[0], nprocs);
#else
  prep();
  test();
#endif

  fibril_rt_exit();
  return verify();
}

#endif /* end of include guard: TEST_H */
