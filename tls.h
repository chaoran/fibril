#ifndef TLS_H
#define TLS_H

#include "deque.h"

typedef struct _tls_t {
  struct tls_s {
    int tid;
    void ** stacks;
    deque_t * deqs;
    deque_t deq;
  } x __attribute__ ((aligned (sizeof(void *))));
  void ** buff[(PAGE_SIZE - sizeof(struct tls_s)) / sizeof(void *)];
} tls_t;

extern tls_t __thread _fibril_tls;
#define FIBRIL_TLS (_fibril_tls.x)
#define FIBRIL_TID (_fibril_tls.x.tid)

#endif /* end of include guard: TLS_H */
