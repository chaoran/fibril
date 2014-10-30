#ifndef JOINT_H
#define JOINT_H

#include "fibrile.h"
#include <stdint.h>

typedef struct {
  int lock;
  int count;
  intptr_t offset;
} joint_t;

typedef fibrile_data_t data_t;

#endif /* end of include guard: JOINT_H */