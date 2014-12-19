#include <stdlib.h>
#include <fibril.h>

static void compute00(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n);
static void compute01(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n);
static void compute10(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n);
static void compute11(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n);

void initialize(float * a, float * b, float ** c, int n)
{
  int i, j;

  for (i = 0; i < n * n; ++i) {
    a[i] = 1.0F;
  }

  for (i = 0; i < n * n; ++i) {
    b[i] = 1.0F;
  }

  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      c[i][j] = 0;
    }
  }
}

int verify(float ** c, int n) {
  int i, j;

  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; j++) {
      if (c[i][j] != n) {
        return 0;
      }
    }
  }

  return 1;
}

void multiply(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj)
{
  int a0 = ai;
  int a1 = ai + 1;

  float s00 = 0.0F;
  float s01 = 0.0F;
  float s10 = 0.0F;
  float s11 = 0.0F;

  int b0 = bi;
  int b1 = bi + 1;

  s00 += a[a0 + aj] * b[b0 + bj];
  s10 += a[a1 + aj] * b[b0 + bj];
  s01 += a[a0 + aj] * b[b0 + bj + 1];
  s11 += a[a1 + aj] * b[b0 + bj + 1];

  s00 += a[a0 + aj + 1] * b[b1 + bj];
  s10 += a[a1 + aj + 1] * b[b1 + bj];
  s01 += a[a0 + aj + 1] * b[b1 + bj + 1];
  s11 += a[a1 + aj + 1] * b[b1 + bj + 1];

  c[ci]    [cj]     += s00;
  c[ci]    [cj + 1] += s01;
  c[ci + 1][cj]     += s10;
  c[ci + 1][cj + 1] += s11;
}

fibril void compute(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n)
{
  if (n == 2) {
    multiply(a, ai, aj, b, bi, bj, c, ci, cj);
  } else {
    int h = n / 2;

    fibril_t fr;
    fibril_init(&fr);

    fibril_fork(&fr, compute00, (a, ai, aj, b, bi, bj, c, ci, cj, h));
    fibril_fork(&fr, compute10, (a, ai, aj, b, bi, bj, c, ci, cj, h));
    fibril_fork(&fr, compute01, (a, ai, aj, b, bi, bj, c, ci, cj, h));
    compute11(a, ai, aj, b, bi, bj, c, ci, cj, h);

    fibril_join(&fr);
  }
}

void compute00(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n)
{
  compute(a, ai, aj,     b, bi,     bj, c, ci, cj, n);
  compute(a, ai, aj + n, b, bi + n, bj, c, ci, cj, n);
}

void compute01(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n)
{
  compute(a, ai, aj,     b, bi,     bj + n, c, ci, cj + n, n);
  compute(a, ai, aj + n, b, bi + n, bj + n, c, ci, cj + n, n);
}

void compute10(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n)
{
  compute(a, ai + n, aj,     b, bi,     bj, c, ci + n, cj, n);
  compute(a, ai + n, aj + n, b, bi + n, bj, c, ci + n, cj, n);
}

void compute11(float * a, int ai, int aj, float * b, int bi, int bj,
    float ** c, int ci, int cj, int n)
{
  compute(a, ai + n, aj,     b, bi,     bj + n, c, ci + n, cj + n, n);
  compute(a, ai + n, aj + n, b, bi + n, bj + n, c, ci + n, cj + n, n);
}

int main(int argc, const char *argv[])
{
  int n = 1024;
  float * a = malloc(sizeof(float [n][n]));
  float * b = malloc(sizeof(float [n][n]));
  float ** c = malloc(sizeof(float * [n]));

  int i;
  for (i = 0; i < n; ++i) {
    c[i] = malloc(sizeof(float [n]));
  }

  initialize(a, b, c, n);

  fibril_rt_init(FIBRIL_NPROCS_ONLN);
  compute(a, 0, 0, b, 0, 0, c, 0, 0, n);
  fibril_rt_exit();

  return verify(c, n) ? 0 : 1;
}

