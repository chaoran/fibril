#ifndef TLS_H
#define TLS_H

#include "fibrile.h"

typedef struct _fibrile_deque_t deque_t;

#define tls_t fibrile_tls_t
#define _tls  fibrile_tls
#define _deq  fibrile_deq

/** Thread id. */
#define _tid (_deq.tid)

/** Process id. */
#define _pid (_deq.pid)


#endif /* end of include guard: TLS_H */
