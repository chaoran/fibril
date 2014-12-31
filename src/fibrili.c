#include <stdlib.h>
#include "proc.h"
#include "sync.h"
#include "debug.h"
#include "stack.h"
#include "fibrile.h"

static int join(fibril_t * frptr)
{
  sync_lock(frptr->lock);

  int count = frptr->count--;
  DEBUG_DUMP(1, "join:", (frptr, "%p"), (count, "%d"));

  if (0 == count) {
    if (frptr->stack.ptr != fibrili_deq.stack) {
      stack_reinstall(frptr);
    }

    sync_unlock(frptr->lock);
    return 1;
  }

  return 0;
}

int fibrili_join(fibril_t * frptr)
{
  if (!join(frptr)) return 0;

  DEBUG_ASSERT(fibrili_deq.stack != frptr->stack.ptr);

  /** Move <b>this</b> frame to the original stack. */
  void ** rsp = frptr->stack.top;
  void ** rbp = rsp - 2;
  void ** src = __builtin_frame_address(0) + sizeof(void *);

  register void ** top asm ("rsp");

  while (src >= top) {
    *(--rsp) = *(src--);
  }

  /** Restore the stack pointer. */
  __asm__ ( "mov\t%0,%%rbp\n\t"
            "mov\t%1,%%rsp"
            : : "g" (rbp), "g" (rsp) );

  free(fibrili_deq.stack);
  fibrili_deq.stack = frptr->stack.ptr;
  return 1;
}

void fibrili_resume(fibril_t * frptr)
{
  if (join(frptr)) {
    fibrili_deq.stack = frptr->stack.ptr;
    proc_resume(frptr, frptr->stack.top);
  } else {
    proc_yield(frptr);
  }
}

