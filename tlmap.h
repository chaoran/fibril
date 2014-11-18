#ifndef TLMAP_H
#define TLMAP_H

#include <stddef.h>

#define __fibril_local__ __attribute__((section(".fibril_tls")))

extern int TID;
extern int PID;
extern ptrdiff_t * TLS_OFFSETS;

extern ptrdiff_t * tlmap_setup(void *, size_t, const char *, int);
extern void tlmap_setup_local(ptrdiff_t *, int, int);
extern void tlmap_init(int nprocs);
extern void tlmap_init_local(int id, int nprocs);

static inline void * tlmap_shptr(void * ptr, int id)
{
  return ptr + TLS_OFFSETS[id];
}

#endif /* end of include guard: TLMAP_H */
