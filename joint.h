#ifndef JOINT_H
#define JOINT_H

#include "fibrile.h"
#include <stdint.h>
#include <string.h>

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

static inline void joint_import(const joint_t * jtptr)
{
  void * dest = jtptr->stptr->top;
  void * addr = jtptr->stptr->top + jtptr->stptr->off;
  size_t size = jtptr->stptr->btm - dest;

  memcpy(dest, addr, size);

  DEBUG_PRINTV("import: jtptr=%p top=%p btm=%p off=%ld\n", jtptr,
      jtptr->stptr->top, jtptr->stptr->btm, jtptr->stptr->off);
}

static inline void joint_export(const joint_t * jtptr)
{
  void * addr = jtptr->stptr->top;
  size_t size = jtptr->stptr->btm - jtptr->stptr->top;
  void * dest = addr + jtptr->stptr->off;

  memcpy(dest, addr, size);

  DEBUG_PRINTV("export: jtptr=%p top=%p btm=%p off=%ld\n", jtptr,
      jtptr->stptr->top, jtptr->stptr->btm, jtptr->stptr->off);
}

#endif /* end of include guard: JOINT_H */
