#include <stdio.h>
#include "safe.h"
#include "debug.h"
#include "fibril.h"

static __attribute__((section(".fibril_shm"))) long table[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 };

long __attribute__ ((hot)) fib(long n)
{
  if (n < 2) return n;

  DEBUG_DUMP(3, "fib:", (n, "%ld"));
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

  DEBUG_DUMP(3, "fib:", (n, "%ld"), (x, "%ld"), (y, "%ld"), (m, "%ld"));
  DEBUG_ASSERT(n >= 2 && n <= 10 && m == table[n]);
  return m;
}

int main(int argc, const char *argv[])
{
  int n = 6;
  long m = fib(n);

  printf("fib(%d) = %ld\n", n, m);
  return 0;
}
