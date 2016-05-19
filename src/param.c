#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "safe.h"
#include "param.h"

size_t PARAM_PAGE_SIZE;
void * PARAM_STACK_ADDR;
size_t PARAM_STACK_SIZE;
int PARAM_NPROCS;

static size_t get_page_size()
{
  int pagesize = sysconf(_SC_PAGESIZE);
  return pagesize;
}

static void get_stack_size(void ** addr, size_t * size)
{
  pthread_attr_t attr;

  pthread_getattr_np(pthread_self(), &attr);
  pthread_attr_getstack(&attr, addr, size);
}

int param_nprocs(int n) {
  int nprocs = 0;

  /** If user provided a positive number, use that number. */
  if (n > 0) {
    nprocs = n;
  }

  if (nprocs == 0) {
    char * env = getenv("FIBRIL_NPROCS");
    if (env) nprocs = atoi(env);
  }

  int max_nprocs = sysconf(_SC_NPROCESSORS_ONLN);

  /**
   * Make sure nprocs is positive and less than or equal to
   * _SC_NPROCESSORS_ONLN.
   */
  if (nprocs <= 0 || nprocs > max_nprocs) {
    nprocs = max_nprocs;
  }

  return nprocs;
}

void param_init(int n)
{
  PARAM_PAGE_SIZE = get_page_size();
  DEBUG_DUMP(2, "init:", (PARAM_PAGE_SIZE, "0x%lx"));

  get_stack_size(&PARAM_STACK_ADDR, &PARAM_STACK_SIZE);
  DEBUG_DUMP(2, "init:", (PARAM_STACK_ADDR, "%p"));
  DEBUG_DUMP(2, "init:", (PARAM_STACK_SIZE, "0x%lx"));

  PARAM_NPROCS = param_nprocs(n);
  DEBUG_DUMP(2, "init:", (PARAM_NPROCS, "%d"));
}

