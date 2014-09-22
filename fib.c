#include "fibril.h"

int main(int argc, const char *argv[])
{
  fibril_init(4);
  fibril_exit();
  return 0;
}
