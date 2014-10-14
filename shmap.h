#ifndef SHMAP_H
#define SHMAP_H

#include <stddef.h>

int    shmap_new(size_t size, const char * name);
void * shmap_map(void * addr, size_t size, int file);
int    shmap_fix(void * addr);
int    shmap_expose(void * addr, size_t size, const char * name);

#endif /* end of include guard: SHMAP_H */
