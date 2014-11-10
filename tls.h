#ifndef TLS_H
#define TLS_H

#include <stdint.h>

#define __fibril_local__ __attribute__((section(".fibril_tls")))

extern int TID;
extern int PID;
extern intptr_t * TLS_OFFSETS;

extern void tls_init(int nprocs);
extern void tls_init_local(int id);

#endif /* end of include guard: TLS_H */
