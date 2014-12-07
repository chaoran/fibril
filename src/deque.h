#ifndef DEQUE_H
#define DEQUE_H

#include "fibrili.h"

typedef struct _fibrili_deque_t deque_t;

typedef struct _frame_t {
  struct _fibril_t * frptr;
  void * stack;
} frame_t;

frame_t deque_steal(deque_t * deq);

#endif /* end of include guard: DEQUE_H */
