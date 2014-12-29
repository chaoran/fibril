#include <fibril.h>

static int fib_fast(int n)
{
  if (n < 2) return n;

  int i = 2, x = 0, y = 0, z = 1;

  do {
    x = y;
    y = z;
    z = x + y;
  } while (i++ < n);

  return z;
}

fibril int fib(int n)
{
  if (n < 2) return n;

  int x, y;
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, x, fib, (n - 1));

  y = fib(n - 2);
  fibril_join(&fr);

  return x + y;
}

int main(int argc, const char *argv[])
{
  fibril_rt_init(FIBRIL_NPROCS_ONLN);
  int n = 42;
  int m = fib(n);

  fibril_rt_exit();
  return (m == fib_fast(n) ? 0 : 1);
}

