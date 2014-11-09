#ifndef JOINT_H
#define JOINT_H

#include <stdint.h>
#include <string.h>

#include "safe.h"
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

static inline joint_t * joint_create()
{
  joint_t * jtptr;

  SAFE_NZCALL(jtptr = malloc(sizeof(joint_t)));
  jtptr->lock = 1;
  jtptr->count = 1;
  jtptr->stptr = &jtptr->stack;

  return jtptr;
}

static inline void joint_import(const joint_t * jtptr)
{
  void * dest = jtptr->stptr->top;
  void * addr = jtptr->stptr->top + jtptr->stptr->off;
  size_t size = jtptr->stptr->btm - dest;

  memcpy(dest, addr, size);

  DEBUG_DUMP(3, "import:", (jtptr, "%p"),
      (jtptr->stptr->top, "%p"),
      (jtptr->stptr->btm, "%p"),
      (jtptr->stptr->off, "%ld"));
}

static inline void joint_export(const joint_t * jtptr)
{
  void * addr = jtptr->stptr->top;
  size_t size = jtptr->stptr->btm - jtptr->stptr->top;
  void * dest = addr + jtptr->stptr->off;

  memcpy(dest, addr, size);

  DEBUG_DUMP(3, "export:", (jtptr, "%p"),
      (jtptr->stptr->top, "%p"),
      (jtptr->stptr->btm, "%p"),
      (jtptr->stptr->off, "%ld"));
}

#endif /* end of include guard: JOINT_H */
