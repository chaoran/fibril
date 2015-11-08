#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "safe.h"
#include "debug.h"
#include "param.h"
#include "stack.h"
#include "stats.h"

static pthread_t * _procs;
static void ** _stacks;
static int _nprocs;

__thread int _tid;

extern void fibrili_init(int id, int nprocs);
extern void fibrili_exit(int id, int nprocs);

static void * __main(void * id)
{
  _tid = (int) (intptr_t) id;

  fibrili_init(_tid, _nprocs);
  return NULL;
}

int fibril_rt_nprocs(int n)
{
  return param_nprocs(n);
}

void handle(int s, siginfo_t * si, void * unused) {
  abort();
}

int fibril_rt_init(int n)
{
  param_init();

  int nprocs = _nprocs = param_nprocs(n);
  DEBUG_DUMP(2, "fibril_rt_init:", (nprocs, "%d"));

  if (_nprocs < 0) return -1;

  /*stack_t altstack;*/
  /*SAFE_RZCALL(posix_memalign(&altstack.ss_sp, PARAM_PAGE_SIZE,*/
        /*PARAM_STACK_SIZE));*/
  /*altstack.ss_flags = 0;*/
  /*altstack.ss_size = PARAM_STACK_SIZE;*/
  /*SAFE_NNCALL(sigaltstack(&altstack, NULL));*/

  struct sigaction sa;
  bzero(&sa, sizeof(sa));
  /*sa.sa_flags = SA_SIGINFO | SA_STACK;*/
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handle;
  SAFE_NNCALL(sigaction(SIGSEGV, &sa, NULL));

  size_t stacksize = PARAM_STACK_SIZE;

  _procs = malloc(sizeof(pthread_t [nprocs]));
  _stacks = malloc(sizeof(void * [nprocs]));

  pthread_attr_t attrs[nprocs];
  int i;

  for (i = 1; i < nprocs; ++i) {
    SAFE_RZCALL(posix_memalign(&_stacks[i], PARAM_PAGE_SIZE, stacksize));
    pthread_attr_init(&attrs[i]);
    pthread_attr_setstack(&attrs[i], _stacks[i], stacksize);
    pthread_create(&_procs[i], &attrs[i], __main, (void *) (intptr_t) i);
    pthread_attr_destroy(&attrs[i]);
  }

  _procs[0] = pthread_self();
  SAFE_RZCALL(posix_memalign(&_stacks[0], PARAM_PAGE_SIZE, stacksize));

  register void * rsp asm ("r15");
  rsp = _stacks[0] + stacksize;

  __asm__ ( "xchg\t%0,%%rsp" : "+r" (rsp) );
  __main((void *) 0);
  __asm__ ( "xchg\t%0,%%rsp" : : "r" (rsp) );

  return 0;
}

int fibril_rt_exit()
{
  fibrili_exit(_tid, _nprocs);

  int i;

  for (i = 1; i < _nprocs; ++i) {
    pthread_join(_procs[i], NULL);
    free(_stacks[i]);
  }

  free(_procs);
  free(_stacks);

  printf("PAGES: %ld %ld\n", STATS_COUNTERS[1].cur, STATS_COUNTERS[1].max);
  return 0;
}

