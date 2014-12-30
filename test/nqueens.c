#include <stdio.h>
#include "test.h"

int n = 14;
int m;

fibril static int nqueens(int * a, int n, int d)
{
  int i, j;

  if (d > 0) {
    for (j = 0; j < d; ++j) {
      int diff = a[j] - a[d];
      int dist = d - j;

      if (diff == 0 || dist == diff || dist + diff == 0) return 0;
    }
  }

  if (++d == n) return 1;

  int res[n];
  int aa[n][d + 1];

  fibril_t fr;
  fibril_init(&fr);

  for (i = 0; i < n; ++i) {
    for (j = 0; j < d; ++j) {
      aa[i][j] = a[j];
    }

    aa[i][d] = i;
    fibril_fork(&fr, res[i], nqueens, (aa[i], n, d));
  }

  fibril_join(&fr);

  int sum = 0;

  for (i = 0; i < n; ++i) {
    sum += res[i];
  }

  return sum;
}

void init() {}
void prep() {}

void test()
{
  m = nqueens(NULL, n, -1);
}

int verify()
{
  static int res[16] = {
    1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680,
    14200, 73712, 365596, 2279184, 14772512
  };

  printf("nqueens(%d)=%d (expected %d)\n", n, m, res[n - 1]);
  return (m != res[n - 1]);
}

