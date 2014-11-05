#ifndef JOINT_H
#define JOINT_H

#include "fibrile.h"
#include <stdint.h>

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

#endif /* end of include guard: JOINT_H */
