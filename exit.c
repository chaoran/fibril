#include <sys/wait.h>

#include "safe.h"
#include "sched.h"
#include "stack.h"
#include "fibrili.h"

int fibril_exit()
{
  int i;
  int status;

  sched_exit();

  for (i = 1; i < _nprocs; ++i) {
    SAFE_FNCALL(waitpid(_pids[i], &status, 0));
    SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  }

  free(_pids);

  for (i = 0; i < _nprocs; ++i) {
    stack_free(_stacks[i], STACK_FREE_ATTOP);
  }

  free(_stacks);

  return 0;
}

