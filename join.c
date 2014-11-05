#include <stdint.h>
#include <string.h>

#include "tls.h"
#include "safe.h"
#include "util.h"
#include "sched.h"
#include "joint.h"
#include "stack.h"

static int commit(void * top, void * btm, intptr_t off,
    const data_t * data, size_t n)
{
  int l = 0;
  int i;

  for (i = 0; i < n; ++i) {
    void * addr = data[i].addr;

    if (addr < top || addr >= btm) continue;

    void * dest = addr + off;
    size_t size = data[i].size;

    SAFE_ASSERT(addr + size <= btm);

    switch (size) {
      case 0: break;
      case 1: *(int8_t  *) dest = *(int8_t  *) addr; break;
      case 2: *(int16_t *) dest = *(int16_t *) addr; break;
      case 4: *(int32_t *) dest = *(int32_t *) addr; break;
      case 8: *(int64_t *) dest = *(int64_t *) addr; break;
      default: memcpy(dest, addr, size);
    }

    l++;
  }

  return l;
}

static void commit_all(joint_t * jtptr, const data_t * data, size_t n)
{
  void * top = jtptr->stptr->top;
  void * btm = jtptr->stptr->btm;
  intptr_t off = jtptr->stptr->off;

  size_t left = n - commit(top, btm, off, data, n);

  joint_t * parent = jtptr->parent;

  SAFE_ASSERT(parent->stptr->top <= btm);
  top = btm;

  while (left > 0 && NULL != (jtptr = parent)) {
    btm = jtptr->stptr->btm;

    if (btm <= top) continue;

    lock(&jtptr->lock);

    off = jtptr->stptr->off;
    left -= commit(top, btm, off, data, n);

    /** Read the parent pointer before unlock. */
    parent = jtptr->parent;
    unlock(&jtptr->lock);

    SAFE_ASSERT(parent->stptr->top <= btm);
    top = btm;
  }
}

static void import(const joint_t * jtptr)
{
  void * dest = jtptr->stptr->top;
  void * addr = dest + jtptr->stptr->off;
  size_t size = jtptr->stptr->btm - jtptr->stptr->top;

  memcpy(dest, addr, size);
  free(addr);

  DEBUG_PRINTV("import: jtptr=%p top=%p btm=%p\n", jtptr,
      jtptr->stptr->top, jtptr->stptr->btm);
}

static void export(const joint_t * jtptr)
{
  void * addr = jtptr->stptr->top;
  size_t size = jtptr->stptr->btm - jtptr->stptr->top;
  void * dest = malloc(size);

  memcpy(dest, addr, size);

  jtptr->stptr->off = dest - addr;

  DEBUG_PRINTV("export: jtptr=%p top=%p btm=%p off=%ld\n", jtptr,
      jtptr->stptr->top, jtptr->stptr->btm, jtptr->stptr->off);
}

int fibrile_join(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  int count = jtptr->count;
  int success = (count == 0);

  if (success) {
    _deq.jtptr = jtptr->parent;
    unlock(&jtptr->lock);
    free(jtptr);

    DEBUG_PRINTC("join (success): frptr=%p jtptr=%p\n", frptr, jtptr);
  } else {
    jtptr->count = count - 1;

    DEBUG_PRINTC("join (failed): frptr=%p jtptr=%p count=%d\n",
        frptr, jtptr, count);
  }

  return success;
}

void fibrile_yield(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  SAFE_ASSERT(jtptr != NULL);

  export(jtptr);

  unlock(&jtptr->lock);
  sched_restart();
}

void fibrile_resume(const fibril_t * frptr, const data_t * data, size_t n)
{
  joint_t * jtptr = _deq.jtptr;
  SAFE_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  if (n > 0) commit_all(jtptr, data, n);

  int count = jtptr->count;

  if (count == 0) {
    import(jtptr);
    _deq.jtptr = jtptr->parent;
    unlock(&jtptr->lock);
    free(jtptr);

    DEBUG_PRINTC("resume (success): frptr=%p jtptr=%p\n", frptr, jtptr);
    sched_resume(frptr);
  } else {
    jtptr->count = count - 1;
    unlock(&jtptr->lock);

    DEBUG_PRINTC("resume (failed): frptr=%p jtptr=%p count=%d\n",
        frptr, jtptr, count);
    sched_restart();
  }
}

