#ifndef DEQUE_H
#define DEQUE_H

#include "page.h"

typedef struct _deque_t {
  int mutex;
  void ** head;
  void ** tail;
  void * buff[0];
} deque_t;

#endif /* end of include guard: DEQUE_H */
