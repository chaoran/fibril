#include "fibril.h"

unsigned long fib(unsigned long n)
{
  if (n < 2) return n;

  unsigned long m;
  FIBRIL_FORK(m = fib(n - 1), (int, n));

  return m + fib(n - 2);
}

int main(int argc, const char *argv[])
{
  return fib(42);
}
