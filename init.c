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
static int _pids[MAX_PROCS];

static int tmain(void * id_)
{
  TID = (int) (intptr_t) id_;
  PID = getpid();

  shmap_init_child(TID);
  return 0;
}

int fibril_init(int nprocs)
{
  /** Initialize globals. */
  _nprocs = nprocs;
  TID = 0;
  PID = getpid();

  /** Initialize shared mappings */
  shmap_init(nprocs);

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    void * stacktop = TLS.stack.maps[i].addr + TLS.stack.size;

    SAFE_RETURN(_pids[i], clone(tmain, stacktop,
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

