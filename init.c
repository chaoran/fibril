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
#include "stack.h"
#include "vtmem.h"
#include "fibril.h"

int _nprocs;
static int _pids[MAX_PROCS];

#define TID FIBRILi_TID
#define PID FIBRILi_PID

static int tmain(void * id_)
{
  int id = (int) (intptr_t) id_;

  vtmem_init_thread(id);
  return 0;
}

int fibril_init(int nprocs)
{
  _nprocs = nprocs;
  vtmem_init(nprocs);

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    SAFE_RETURN(_pids[i], clone(tmain, stack_top(i),
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD,
          (void *) (intptr_t) i));
    DEBUG_PRINT_INFO("clone: tid=%d pid=%d stack_top=%p\n",
        i, _pids[i], stack_top(i));
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

