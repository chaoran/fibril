#include <sys/wait.h>

#include "conf.h"
#include "safe.h"
#include "stack.h"
#include "fibril.h"
#include "fibrili.h"

extern int _pids[MAX_PROCS];
extern int _nprocs;
extern void ** _stacks;

int fibril_exit()
{
  int i;
  int status;

  for (i = 1; i < _nprocs; ++i) {
    SAFE_FNCALL(waitpid(_pids[i], &status, 0));
    SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  }

  for (i = 0; i < _nprocs; ++i) {
    stack_free(_stacks[i]);
  }

  return FIBRIL_SUCCESS;
}

