#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "safe.h"
#include "param.h"
#include "fibril.h"

size_t PARAM_PAGE_SIZE;
void * PARAM_STACK_ADDR;
size_t PARAM_STACK_SIZE;
int PARAM_NUM_PROCS;

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

static int get_num_procs(int n)
{
  int nprocs;
  int nponln = sysconf(_SC_NPROCESSORS_ONLN);
  char * env;

  switch (n) {
    case FIBRIL_NPROCS_ONLN:
      nprocs = nponln;
      break;
    case FIBRIL_NPROCS:
      if ((env = getenv("FIBRIL_NPROCS"))) {
        nprocs = atoi(env);
      } else {
        nprocs = -1;
      }
      break;
    default:
      nprocs = n;
  }

  SAFE_ASSERT(nprocs > 0 && nprocs <= nponln);
  return nprocs;
}

int param_init(int nprocs)
{
  PARAM_NUM_PROCS = get_num_procs(nprocs);
  DEBUG_DUMP(2, "init:", (PARAM_NUM_PROCS, "%d"));

  PARAM_PAGE_SIZE = get_page_size();
  DEBUG_DUMP(2, "init:", (PARAM_PAGE_SIZE, "0x%lx"));

  get_stack_size(&PARAM_STACK_ADDR, &PARAM_STACK_SIZE);
  DEBUG_DUMP(2, "init:", (PARAM_STACK_ADDR, "%p"));
  DEBUG_DUMP(2, "init:", (PARAM_STACK_SIZE, "0x%lx"));

  return PARAM_NUM_PROCS;
}

