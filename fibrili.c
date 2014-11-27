#include <stdlib.h>
#include "sync.h"
#include "debug.h"
#include "sched.h"
#include "fibril.h"

void fibrili_resume(fibril_t * frptr)
{
  int count;

  sync_lock(frptr->lock);
  count = frptr->count--;

  DEBUG_DUMP(3, "resume:", (frptr, "%p"), (count, "%d"));

  if (count == 0) {
    sync_unlock(frptr->lock);

    free(frptr->stack);
    sched_resume(frptr);
  } else {
    sched_restart(frptr);
  }
}

void fibrili_yield(fibril_t * frptr)
{
  DEBUG_DUMP(3, "yield:", (frptr, "%p"), (frptr->count, "%d"));
  frptr->regs.rip = __builtin_extract_return_addr(__builtin_return_address(0));
  sched_restart(frptr);
}

