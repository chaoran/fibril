#include <stdio.h>
#include "fibril.h"

long __attribute__ ((hot)) fib(long n)
{
  if (n < 2) return n;

  fibril_t fr;
  fibril_new(&fr);

  long x, y;

  do {
    fibrile_save(&fr, &&AFTER_FORK);
    fibrile_push(&fr);
    x = fib(n - 1);

    if (!fibrile_pop()) {
      fibrile_data_t data[1] = { { &x, sizeof(x) } };
      fibrile_join(&fr, data, 1);
    }
AFTER_FORK: break;
  } while (0);

  y = fib(n - 2);

  fibril_join(&fr);
  return x + y;
}

int main(int argc, const char *argv[])
{
  int n = 10;

  fibril_init(4);
  printf("fib(%d) = %ld\n", n, fib(n));
  fibril_exit();

  return 0;
}
