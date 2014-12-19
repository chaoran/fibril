#include <stdio.h>
#include <fibril.h>

static int x0;
static int x1;
static double area;
static const double epsilon = 1.0e-9;

static double f(double x)
{
  return (x * x + 1.0) * x;
}

static fibril
double integrate(double x1, double y1, double x2, double y2, double area)
{
  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = f(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < epsilon && area - area_x1x2 < epsilon) {
    return area_x1x2;
  }

  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, area_x1x0, integrate, (x1, y1, x0, y0, area_x1x0));
  area_x0x2 = integrate(x0, y0, x2, y2, area_x0x2);

  fibril_join(&fr);
  return area_x1x0 + area_x0x2;
}

void init()
{
  x0 = 0;
  x1 = 10000;
}

void test()
{
  area = integrate(x0, f(x0), x1, f(x1), 0);
}

int verify()
{
  static const double expect = 2500000050000000.0;

  if (area - expect < epsilon && expect - area < epsilon) {
    return 1;
  }

  printf("expected: %lf but result is: %lf\n", expect, area);
  return 0;
}

int main(int argc, char* argv[])
{
  init();

  fibril_rt_init(FIBRIL_NPROCS_ONLN);
  test();
  fibril_rt_exit();

  return !verify();
}

