#ifndef DEQUE_H
#define DEQUE_H

#include "joint.h"
#include "fibril.h"

typedef struct _fibrile_deque_t deque_t;

extern struct _fibril_t * deque_steal(deque_t * deq, int vic, joint_t * jtp);

#endif /* end of include guard: DEQUE_H */
