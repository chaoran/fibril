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

static void bench(const char * name)
{
  static iter = 10;

  float times[iter];
  printf("===========================================\n");
  printf("  Benchmark: %s\n", strrchr(name, '/') + 1);
  printf("  Input size: %d\n", n);
  printf("  Number of iterations: %d\n", iter);
  printf("  Number of processors: %d\n", fibril_rt_nprocs(0));

  int i;
  for (i = 0; i < iter; ++i) {
    prep();
    clock_t t = clock();
    test();
    t = clock() - t;
    times[i] = (float) t / CLOCKS_PER_SEC;
    printf("  #%d execution time: %fs\n", i + 1, times[i]);
  }

  float max = 0.0;
  float min = FLT_MAX;
  float sum = 0.0;

  for (i = 0; i < iter; ++i) {
    if (times[i] > max) max = times[i];
    if (times[i] < min) min = times[i];
    sum += times[i];
  }

  printf("  Maximum execution time: %fs\n", max);
  printf("  Minimum execution time: %fs\n", min);
  printf("  Average execution time: %fs\n", sum / iter);
  printf("===========================================\n");
}

#endif

int main(int argc, const char *argv[])
{
  if (argc > 1 && (argc = atoi(argv[1])) > 0) {
    n = argc;
  }

  init();
  fibril_rt_init(FIBRIL_NPROCS_ONLN);

#ifdef BENCHMARK
  bench(argv[0]);
#else
  prep();
  test();
#endif

  fibril_rt_exit();
  return verify();
}

#endif /* end of include guard: TEST_H */
