#ifndef TLS_H
#define TLS_H

#include "fibrile.h"

#define __fibril_local__ __attribute__((section(".fibril_tls")))

extern int TID;
extern int PID;

#endif /* end of include guard: TLS_H */
