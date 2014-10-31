#include <stdint.h>
#include <string.h>

#include "tls.h"
#include "safe.h"
#include "util.h"
#include "sched.h"
#include "joint.h"
#include "stack.h"

static inline
void write_update(intptr_t offset, const data_t * data, size_t n)
{
  int i;
  for (i = 0; i < n; ++i) {
    void * src  = data->addr;
    void * dest = src + offset;
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

int fibrile_join(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  int count = jtptr->count;
  int success = (count == 0);

  if (success) {
    unlock(&jtptr->lock);
    free(jtptr);

    DEBUG_PRINTC("join (success): frptr=%p\n", frptr);
  } else {
    jtptr->count = count - 1;

    DEBUG_PRINTC("join (failed): frptr=%p count=%d\n", frptr, count);
  }

  return success;
}

void fibrile_yield(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  jtptr->stack = stack_new(frptr->rsp);
  stack_export(jtptr->stack, frptr->rsp);

  unlock(&jtptr->lock);
  sched_restart();
}

void fibrile_resume(const fibril_t * frptr, const data_t * data, size_t n)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  int count = jtptr->count;

  if (count == 0) {
    write_update(jtptr->stack - frptr->rsp, data, n);
    stack_import(frptr->rsp, jtptr->stack);

    unlock(&jtptr->lock);
    free(jtptr);

    DEBUG_PRINTC("resume (success): frptr=%p rip=%p\n", frptr, frptr->rip);
    sched_resume(frptr);
  } else {
    jtptr->count = count - 1;

    write_update(jtptr->stack - frptr->rsp, data, n);
    unlock(&jtptr->lock);

    DEBUG_PRINTC("resume (failed): frptr=%p count=%d\n", frptr, count);
    sched_restart();
  }
}

