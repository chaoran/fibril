#ifndef TLS_H
#define TLS_H

#include "fibrile.h"

#define tls_t fibrile_tls_t
#define _tls  fibrile_tls

/** Thread id. */
#define _tid (_tls.deq.tid)

/** Process id. */
#define _pid (_tls.deq.pid)


#endif /* end of include guard: TLS_H */
