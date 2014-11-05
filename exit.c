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
  stack_finalize(_nprocs);

  return 0;
}

