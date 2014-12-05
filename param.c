#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "debug.h"
#include "param.h"

size_t PARAM_STACK_SIZE;
int PARAM_NUM_PROCS;

static size_t get_stack_size()
{
  size_t size;
  pthread_attr_t attr;

  pthread_getattr_np(pthread_self(), &attr);
  pthread_attr_getstacksize(&attr, &size);

  return size;
}

static int get_num_procs()
{
  int nprocs = sysconf(_SC_NPROCESSORS_ONLN);

  char * str;
  if ((str = getenv("FIBRIL_NPROCS"))) {
    int n = atoi(str);

    if (n <= nprocs && n > 0) {
      nprocs = n;
    } else {
      fprintf(stderr, "Invalid FIBRIL_NPROCS (should be 1~%d)\n", nprocs);
      exit(1);
    }
  }

  return nprocs;
}

__attribute__((constructor)) void init(void)
{
  PARAM_STACK_SIZE = get_stack_size();
  PARAM_NUM_PROCS = get_num_procs();

  DEBUG_DUMP(2, "init:", (PARAM_NUM_PROCS, "%d"), (PARAM_STACK_SIZE, "0x%lx"));
}
