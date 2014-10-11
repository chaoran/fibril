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
#include "shmap.h"
#include "fibril.h"

static int _nprocs;
static int _tids[MAX_PROCS];
static shmap_t * _stacks[MAX_PROCS];

tls_t __thread _fibril_tls;

static int tmain(void * id_)
{
  FIBRIL_TID = (int) (intptr_t) id_;
  return 0;
}

int fibril_init(int nprocs)
{
  /** Initialize globals. */
  _nprocs = nprocs;
  _tids[0] = getpid();

  /** Initialize shared mappings */
  shmap_init(nprocs, _stacks);

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    SAFE_RETURN(_tids[i], clone(tmain, _stacks[i]->addr + _stacks[i]->size,
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD, (void *) (intptr_t) i));
  }

  tmain((void *) 0);
  return FIBRIL_SUCCESS;
}

int fibril_exit()
{
  int i;
  int status;

  for (i = 1; i < _nprocs; ++i) {
    SAFE_FNCALL(waitpid(_tids[i], &status, 0));
    SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  }

  return FIBRIL_SUCCESS;
}

