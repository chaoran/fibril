#ifndef DEQUE_H
#define DEQUE_H

#include "fibril.h"

typedef struct _fibrile_deque_t deque_t;

#define DEQ (fibrile_tls.deq)

extern fibril_t * deque_steal(deque_t * deq);

#endif /* end of include guard: DEQUE_H */
