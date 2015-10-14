#ifndef TEST_H
#define TEST_H

extern void init();
extern void prep();
extern void test();
extern int verify();

extern int n;

#include <fibril.h>

#ifdef BENCHMARK

#include <time.h>
#include <stdio.h>
#include <float.h>
#include <string.h>

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
    clock_t t = clock();
    test();
    t = clock() - t;
    times[i] = (float) t / CLOCKS_PER_SEC / nprocs;
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
