#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

#include "tls.h"
#include "conf.h"
#include "safe.h"
#include "debug.h"
#include "deque.h"
#include "stack.h"
#include "vtmem.h"
#include "fibril.h"

static int _nprocs;
static int _pids[MAX_PROCS];

static int tmain(void * id_)
{
  _tls.tid = (int) (intptr_t) id_;
  _tls.pid = getpid();

  stack_init_thread(_tls.tid);
  deque_init_thread(_tls.tid, &_tls.deque);

  return 0;
}

int fibril_init(int nprocs)
{
  _nprocs = nprocs;
  _tls.tid = 0;
  _tls.pid = getpid();

  vtmem_init(nprocs);
  stack_init(nprocs);
  deque_init(nprocs, &_tls.deque);

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    SAFE_RETURN(_pids[i], clone(tmain, stack_top(i),
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD,
          (void *) (intptr_t) i));
  }

  tmain((void *) 0);
  return FIBRIL_SUCCESS;
}

int fibril_exit()
{
  int i;
  int status;

  for (i = 1; i < _nprocs; ++i) {
    SAFE_FNCALL(waitpid(_pids[i], &status, 0));
    SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  }

  return FIBRIL_SUCCESS;
}

