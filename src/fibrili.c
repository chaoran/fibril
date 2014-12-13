#include <stdlib.h>
#include "sync.h"
#include "debug.h"
#include "stack.h"
#include "fibril.h"

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

__attribute__((noinline, optimize("-O3", "-fno-omit-frame-pointer")))
int fibrili_join(fibril_t * frptr)
{
  if (!join(frptr)) return 0;

  DEBUG_ASSERT(fibrili_deq.stack != frptr->stack.top);

  /** Move <b>this</b> frame to the original stack. */
  void ** top = frptr->stack.top;
  void ** rbp = __builtin_frame_address(0) + sizeof(void *);

  register void ** rsp asm ("rsp");

  while (rbp >= rsp) {
    *(--top) = *(rbp--);
  }

  /** Restore the stack pointer. */
  __asm__ ( "mov\t%0,%%rsp" : : "g" (top) );

  free(fibrili_deq.stack);
  fibrili_deq.stack = frptr->stack.ptr;
  return 1;
}

void fibrili_resume(fibril_t * frptr)
{
  if (join(frptr)) {
    fibrili_deq.stack = frptr->stack.ptr;
    fibrili_longjmp(&frptr->state, frptr->stack.top);
  } else {
    fibrili_yield(frptr);
  }
}

