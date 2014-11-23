#include <stdint.h>
#include <string.h>

#include "safe.h"
#include "util.h"
#include "sched.h"
#include "joint.h"
#include "stack.h"
#include "tlmap.h"

static void commit(joint_t * jtptr, const data_t * data, size_t n)
{
  if (n == 0) {
    unlock(&jtptr->lock);
    return;
  }

  size_t left = n;
  int locked = 1;
  int commited[n];

  memset(commited, 0, n * sizeof(int));

  do {
    void *  start = jtptr->stptr->top;
    void *    end = jtptr->stptr->btm;
    ptrdiff_t off = jtptr->stptr->off;

    int i;
    for (i = 0; i < n; ++i) {
      if (commited[i]) continue;

      void * addr = data[i].addr;

      if (addr >= start && addr < end) {
        void * dest = addr + off;
        size_t size = data[i].size;

        DEBUG_ASSERT(addr + size <= end);

        if (!locked) {
          lock(&jtptr->lock);
          locked = 1;
        }

        switch (size) {
          case 0: break;
          case 1: *(int8_t  *) dest = *(int8_t  *) addr; break;
          case 2: *(int16_t *) dest = *(int16_t *) addr; break;
          case 4: *(int32_t *) dest = *(int32_t *) addr; break;
          case 8: *(int64_t *) dest = *(int64_t *) addr; break;
          default: memcpy(dest, addr, size);
        }

        DEBUG_DUMP(3, "commit:", (jtptr, "%p"), (addr, "%p"), (dest, "%p"),
            (*(long *) addr, "%ld"));

        commited[i] = 1;
        left--;
      }
    }

    if (locked) {
      unlock(&jtptr->lock);
      locked = 0;
    }
  } while (left > 0 && (jtptr = jtptr->parent));
}

int fibrile_join(const fibril_t * frptr)
{
  joint_t * jtptr = frptr->jtp;
  DEBUG_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  int count = jtptr->count;
  int success = (count == 0);

  if (success) {
    fibrile_deq.jtptr = jtptr->parent;
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
  joint_t * jtptr = fibrile_deq.jtptr;
  DEBUG_ASSERT(jtptr != NULL);

  lock(&jtptr->lock);

  if (--jtptr->count < 0) {
    DEBUG_DUMP(1, "resume (success):", (frptr, "%p"), (jtptr, "%p"));

    commit(jtptr, data, n);
    sched_resume(jtptr, frptr);
  } else {
    DEBUG_DUMP(1, "resume (failed):", (frptr, "%p"), (jtptr, "%p"),
        (jtptr->count, "%d"));

    commit(jtptr, data, n);
    sched_restart();
  }
}

