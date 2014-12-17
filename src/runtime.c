#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "proc.h"
#include "debug.h"
#include "param.h"
#include "fibril.h"

static pthread_t * _procs;
static void ** _stacks;

static void * __main(void * id)
{
  int tid = (int) (intptr_t) id;
  int nprocs = PARAM_NUM_PROCS;

  proc_start(tid, nprocs);
  return NULL;
}

int fibril_rt_init(int nprocs)
{
  nprocs = param_init(nprocs);
  DEBUG_DUMP(2, "fibril_rt_init:", (nprocs, "%d"));

  size_t stacksize = PARAM_STACK_SIZE;

  _procs = malloc(sizeof(pthread_t [nprocs]));
  _stacks = malloc(stacksize);

  pthread_attr_t attrs[nprocs];
  int i;

  for (i = 1; i < nprocs; ++i) {
    _stacks[i] = malloc(stacksize);
    pthread_attr_init(&attrs[i]);
    pthread_attr_setstack(&attrs[i], _stacks[i], stacksize);
    pthread_create(&_procs[i], &attrs[i], __main, (void *) (intptr_t) i);
    pthread_attr_destroy(&attrs[i]);
  }

  _procs[0] = pthread_self();
  _stacks[0] = malloc(stacksize);

  register void * rsp asm ("r15");
  rsp = _stacks[0] + stacksize;

  __asm__ ( "xchg\t%0,%%rsp" : "+r" (rsp) );
  __main((void *) 0);
  __asm__ ( "xchg\t%0,%%rsp" : : "r" (rsp) );

  return FIBRIL_SUCCESS;
}

int fibril_rt_exit()
{
  proc_stop();

  int i;
  int nprocs = PARAM_NUM_PROCS;

  for (i = 1; i < nprocs; ++i) {
    pthread_join(_procs[i], NULL);
    free(_stacks[i]);
  }

  free(_procs);
  free(_stacks);

  return FIBRIL_SUCCESS;
}

