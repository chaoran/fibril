#include <stdio.h>
#include "safe.h"
#include "debug.h"
#include "fibril.h"

long __attribute__ ((hot)) fib(long n)
{
  if (n < 2) return n;

  DEBUG_PRINT("computing fib(%ld)\n", n);
  SAFE_ASSERT(n <= 10 && n >= 2);
  fibril_t fr;
  fibril_make(&fr);

  long x, y, m;

  do {
    fibrile_save(&fr, &&AFTER_FORK);
    fibrile_push(&fr);
    x = fib(n - 1);

    if (!fibrile_pop()) {
      fibrile_data_t data[1] = { { &x, sizeof(x) } };
      fibrile_resume(&fr, data, 1);
    }
AFTER_FORK: break;
  } while (0);

  y = fib(n - 2);

  fibril_join(&fr);
  m = x + y;

  DEBUG_PRINT("fib(%ld)=%ld\n", n, m);
  return m;
}

int main(int argc, const char *argv[])
{
  int n = 10;

  fibril_init(4);
  printf("fib(%d) = %ld\n", n, fib(n));
  fibril_exit();

  return 0;
}
