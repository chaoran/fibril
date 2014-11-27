#include <stdlib.h>
#include "sync.h"
#include "sched.h"
#include "fibril.h"

int fibrili_join(fibril_t * frptr)
{
  /** Free the extra stack. */
  free(frptr->stack);

  sync_lock(frptr->lock);

  int success = (frptr->count-- == 0);

  if (success) {
    sync_unlock(frptr->lock);
  }

  /** Restore the original rsp. */
  __asm__ ( "mov\t%0,%%rsp" : : "g" (frptr->regs.rsp) );
  return success;
}

void fibrili_resume(fibril_t * frptr)
{
  int count;

  sync_lock(frptr->lock);
  count = frptr->count--;

  if (count == 0) {
    sync_unlock(frptr->lock);
    sched_resume(frptr);
  } else {
    sched_restart(frptr);
  }
}

void fibrili_yield(fibril_t * frptr)
{
  sched_restart(frptr);
}

