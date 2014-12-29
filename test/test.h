#ifndef TEST_H
#define TEST_H

extern void init();
extern void test();
extern int verify();

extern int n;

#include <fibril.h>

int main(int argc, const char *argv[])
{
  if (argc > 1 && (argc = atoi(argv[1])) > 0) {
    n = argc;
  }

  init();
  fibril_rt_init(FIBRIL_NPROCS_ONLN);

  test();

  fibril_rt_exit();
  return verify();
}

#endif /* end of include guard: TEST_H */
