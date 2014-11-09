#include <sys/wait.h>

#include "safe.h"
#include "sched.h"
#include "stack.h"
#include "tlmap.h"

extern int _nprocs;

void __attribute__ ((destructor)) finalize()
{
  sched_exit(TID);

  /*int i;*/
  /*int status;*/
  /*int nprocs = _nprocs;*/

  /*for (i = 1; i < nprocs; ++i) {*/
    /*if (wait(&status) == -1) {*/
      /*SAFE_ASSERT(errno == ECHILD);*/
      /*break;*/
    /*}*/

    /*DEBUG_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));*/
  /*}*/

  if (TID == 0) stack_free();
}

