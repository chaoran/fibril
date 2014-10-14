#ifndef TLS_H
#define TLS_H

#include "conf.h"
#include "page.h"
#include "deque.h"
#include "stack.h"

typedef struct _tls_t {
  int tid;
  int pid;
  stack_info_t stack;
  deque_t * deque;
} tls_t __attribute__((aligned (PAGE_SIZE)));

extern tls_t _tls;

#endif /* end of include guard: TLS_H */
