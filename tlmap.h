#ifndef TLMAP_H
#define TLMAP_H

#include <stdint.h>

#define __fibril_local__ __attribute__((section(".fibril_tls")))

extern int TID;
extern int PID;
extern intptr_t * TLS_OFFSETS;

void tlmap_init();
void tlmap_free();
void tlmap_init_local(int id);

#endif /* end of include guard: TLMAP_H */
