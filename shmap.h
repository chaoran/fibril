#ifndef SHMAP_H
#define SHMAP_H

#include <stddef.h>

typedef struct _shmap_t {
  struct _shmap_t * next;
  int    lock;
  int    file;
  void * addr;
  size_t size;
} shmap_t __attribute ((aligned (sizeof(void *))));

int       shmap_new(size_t size, const char * name);
shmap_t * shmap_map(int file, void * addr, size_t size);
int       shmap_fix(void * addr);

void shmap_init(int nprocs);
void shmap_init_child(int id);

#endif /* end of include guard: SHMAP_H */
