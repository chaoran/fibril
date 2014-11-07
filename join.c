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

    DEBUG_ASSERT(addr + size <= btm);

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

  DEBUG_ASSERT(parent->stptr->top <= btm);
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

    DEBUG_ASSERT(parent->stptr->top <= btm);
    top = btm;
  }
}

int fibrile_join(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  DEBUG_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  int count = jtptr->count;
  int success = (count == 0);

  if (success) {
    DEQ.jtptr = jtptr->parent;
    unlock(&jtptr->lock);
    free(jtptr);

    DEBUG_DUMP(3, "join (success):", (frptr, "%p"), (jtptr, "%p"));
  } else {
    jtptr->count = count - 1;

    DEBUG_DUMP(3, "join (failed):", (frptr, "%p"), (jtptr, "%p"),
        (jtptr->count, "%d"));
  }

  return success;
}

void fibrile_yield(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  DEBUG_ASSERT(jtptr != NULL);

  void * dest;
  SAFE_NZCALL(dest = malloc(jtptr->stptr->btm - jtptr->stptr->top));
  jtptr->stptr->off = dest - jtptr->stptr->top;

  joint_export(jtptr);

  unlock(&jtptr->lock);
  sched_restart();
}

void fibrile_resume(const fibril_t * frptr, const data_t * data, size_t n)
{
  joint_t * jtptr = DEQ.jtptr;
  DEBUG_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  if (n > 0) commit_all(jtptr, data, n);

  int count = jtptr->count;

  if (count == 0) {
    DEQ.jtptr = jtptr->parent;

    joint_import(jtptr);
    unlock(&jtptr->lock);

    free(jtptr->stptr->top + jtptr->stptr->off);
    free(jtptr);

    DEBUG_DUMP(1, "resume (success):", (frptr, "%p"), (jtptr, "%p"));
    sched_resume(frptr);
  } else {
    jtptr->count = count - 1;
    unlock(&jtptr->lock);

    DEBUG_DUMP(1, "resume (failed):", (frptr, "%p"), (jtptr, "%p"),
        (jtptr->count, "%d"));
    sched_restart();
  }
}

