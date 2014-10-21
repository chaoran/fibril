#ifndef FIBRILI_H
#define FIBRILI_H

#include "conf.h"
#include "fibrile.h"

/** Types. */
typedef FIBRILe(tls_t) tls_t;
typedef FIBRILe(deque_t) deque_t;

/** Variables. */
#define _tls FIBRILe_TLS
#define _tid FIBRILe_TID
#define _pid FIBRILe_PID
#define _deq FIBRILe_DEQ

/** Functions. */
#define deque_push FIBRILe(push)
#define deque_pop  FIBRILe(pop)

#endif /* end of include guard: FIBRILI_H */
