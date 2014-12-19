#include <stdlib.h>
#include <fibril.h>

fibril void quicksort(int * a, size_t n)
{
  if (n < 2) return;

  int pivot = a[n / 2];

  int *left  = a;
  int *right = a + n - 1;

  while (left <= right) {
    if (*left < pivot) {
      left++;
    } else if (*right > pivot) {
      right--;
    } else {
      int tmp = *left;
      *left = *right;
      *right = tmp;
      left++;
      right--;
    }
  }

  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, quicksort, (a, right - a + 1));
  quicksort(left, a + n - left);

  fibril_join(&fr);
}

static int verify(int * a, size_t n)
{
  if (n < 2) return 1;

  int prev = a[0];
  int i;
  for (i = 1; i < n; ++i) {
    if (prev > a[i]) return 0;
    prev = a[i];
  }

  return 1;
}

static void init(int * a, size_t n)
{
  int i;
  for (i = 0; i < n; ++i) {
    a[i] = rand();
  }
}

int main(int argc, const char *argv[])
{
  int n = 10000000;
  int * a = malloc(sizeof(int [n]));

  init(a, n);

  fibril_rt_init(FIBRIL_NPROCS_ONLN);
  quicksort(a, n);
  fibril_rt_exit();

  return !verify(a, n);
}

