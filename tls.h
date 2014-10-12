#ifndef TLS_H
#define TLS_H

#include "conf.h"
#include "deque.h"

typedef struct _tls_t {
  struct tls {
    int tid;
    void ** stacks;
    deque_t * deqs;
    deque_t deq;
  } x __attribute__ ((aligned (sizeof(void *))));
  void ** buff[
    (NUM_TLS_PAGES * PAGE_SIZE - sizeof(struct tls)) / sizeof(void *)
  ];
} tls_t __attribute__((aligned (PAGE_SIZE)));

extern tls_t _tls;
#define TLS (_tls.x)
#define TID (_tls.x.tid)

#endif /* end of include guard: TLS_H */
