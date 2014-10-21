#include <sys/wait.h>

#include "conf.h"
#include "safe.h"
#include "fibril.h"
#include "fibrili.h"

extern int _pids[MAX_PROCS];

int fibril_exit()
{
  int i;
  int status;

  for (i = 1; _pids[i] != 0; ++i) {
    SAFE_FNCALL(waitpid(_pids[i], &status, 0));
    SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  }

  return FIBRIL_SUCCESS;
}

