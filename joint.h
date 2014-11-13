#ifndef JOINT_H
#define JOINT_H

#include <stdint.h>
#include <string.h>

#include "safe.h"
#include "deque.h"
#include "stack.h"
#include "fibrile.h"

typedef struct _fibrile_joint_t {
  int lock;
  int count;
  struct _fibrile_joint_t * parent;
  struct _stack_t {
    void * top;
    void * btm;
    intptr_t off;
  } stack;
  struct _stack_t * stptr;
} joint_t;

typedef fibrile_data_t data_t;

extern joint_t _joint;

static inline joint_t * joint_create(
    const fibril_t * frptr, const deque_t * deq, int tid)
{
  DEBUG_ASSERT(frptr->jtp == NULL);
  DEBUG_ASSERT(frptr->rsp != NULL);
  DEBUG_ASSERT(deq->jtptr != NULL);

  joint_t * jtptr;

  SAFE_NZCALL(jtptr = malloc(sizeof(joint_t)));
  jtptr->lock = 1;
  jtptr->count = 1;

  joint_t * parent = deq->jtptr;
  jtptr->parent = parent;

  if (parent->stptr->off == STACK_OFFSETS[tid]) {
    jtptr->stptr = parent->stptr;
  } else {
    jtptr->stack.btm = parent->stptr->top;
    jtptr->stptr = &jtptr->stack;
  }

  jtptr->stptr->top = frptr->rsp;
  jtptr->stptr->off = STACK_OFFSETS[TID];

  DEBUG_DUMP(3, "create:", (jtptr, "%p"), (parent, "%p"),
      (jtptr->stptr->top, "%p"), (jtptr->stptr->btm, "%p"));
  return jtptr;
}


static inline void joint_import(const joint_t * jtptr)
{
  void * dest = jtptr->stptr->top;
  void * addr = jtptr->stptr->top + jtptr->stptr->off;
  size_t size = jtptr->stptr->btm - dest;

  memcpy(dest, addr, size);

  DEBUG_DUMP(3, "import:", (jtptr, "%p"), (dest, "%p"),
      (size, "%lu"), (addr, "%p"));
}

static inline void joint_export(const joint_t * jtptr)
{
  void * addr = jtptr->stptr->top;
  size_t size = jtptr->stptr->btm - jtptr->stptr->top;
  void * dest = addr + jtptr->stptr->off;

  memcpy(dest, addr, size);

  DEBUG_DUMP(3, "export:", (jtptr, "%p"),
      (addr, "%p"), (size, "%lu"), (dest, "%p"));
}

#endif /* end of include guard: JOINT_H */
