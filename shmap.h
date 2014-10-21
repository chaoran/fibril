#ifndef SHMAP_H
#define SHMAP_H

#include <stddef.h>

void   shmap_init(int nprocs);
int    shmap_open(size_t size, const char * name);
int    shmap_copy(void * addr, size_t size, const char * name);
void * shmap_mmap(void * addr, size_t size, int file);

#endif /* end of include guard: SHMAP_H */
