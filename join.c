#include <stdint.h>
#include <string.h>

#include "tls.h"
#include "safe.h"
#include "util.h"
#include "sched.h"
#include "joint.h"
#include "stack.h"

static inline
void update_stack(size_t offset, const data_t * data, size_t n)
{
  int i;
  for (i = 0; i < n; ++i) {
    void * src  = data->addr;
    void * dest = adjust(src, offset);
    size_t size = data->size;

    switch (size) {
      case 0: break;
      case 1: *(int8_t  *) dest = *(int8_t  *) src; break;
      case 2: *(int16_t *) dest = *(int16_t *) src; break;
      case 4: *(int32_t *) dest = *(int32_t *) src; break;
      case 8: *(int64_t *) dest = *(int64_t *) src; break;
      default: memcpy(dest, src, size);
    }
  }
}

static inline
int join(joint_t * jtptr, const fibril_t * frptr, const data_t * data, size_t n)
{
  int success = (jtptr->count-- == 0);

  if (success) {
    size_t offset = jtptr->offset;

    if (offset != stack_offset(_tid)) {
      /** Update remote stack. */
      update_stack(offset, data, n);

      /** Copy stack in. */
      void * stack = adjust(frptr->rsp, offset);
      stack_import(frptr->rsp, stack);
      stack_free(stack, STACK_FREE_INPLACE);

      DEBUG_PRINTV(
          "import: frptr=%p frptr->rsp=%p stack=%p rfrptr=%p rfrptr->rsp=%p\n",
          frptr, frptr->rsp, stack,
          adjust(frptr, offset),
          ((fibril_t *) adjust(frptr, offset))->rsp);
    }
  } else {
    if (jtptr->offset == stack_offset(_tid)) {
      /** Copy stack out. */
      void * stack = stack_new(frptr->rsp);
      jtptr->offset = (void *) frptr->rsp - stack;
      stack_export(stack, frptr->rsp);

      DEBUG_PRINTV(
          "export: frptr=%p frptr->rsp=%p stack=%p rfrptr=%p rfrptr->rsp=%p\n",
          frptr, frptr->rsp, stack,
          adjust(frptr, jtptr->offset),
          ((fibril_t *) adjust(frptr, jtptr->offset))->rsp);
    } else {
      /** Write my updates to remote stack. */
      update_stack(jtptr->offset, data, n);
    }
  }

  return success;
}

__attribute__ ((noreturn))
int fibrile_join(const fibril_t * frptr, const data_t * data, size_t n)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  if (join(jtptr, frptr, data, n)) {
    free(jtptr);
    unlock(&jtptr->lock);

    DEBUG_PRINTC("join (slow): frptr=%p rip=%p\n", frptr, frptr->rip);
    SAFE_ASSERT(frptr->rip != NULL);
    sched_resume(frptr);
  } else {
    unlock(&jtptr->lock);

    DEBUG_PRINTC("join (failed): frptr=%p count=%d\n", frptr, jtptr->count);
    sched_restart();
  }
}

int _fibril_join(fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  if (join(jtptr, frptr, NULL, 0)) {
    unlock(&jtptr->lock);
    free(jtptr);

    DEBUG_PRINTC("join (fast): frptr=%p\n", frptr);
    return 1;
  }

  fibril_t * rfrptr = adjust(frptr, jtptr->offset);
  frame_t * base = this_frame();
  rfrptr->rsp = base->rsp;
  fibrile_save(rfrptr, base->rip);

  unlock(&jtptr->lock);

  DEBUG_PRINTC("join (failed): frptr=%p count=%d rip=%p\n",
      frptr, jtptr->count, rfrptr->rip);
  sched_restart();
}

