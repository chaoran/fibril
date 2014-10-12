#ifndef SHMAP_H
#define SHMAP_H

#include <stddef.h>

typedef struct _shmap_t {
  int fd;
  void * addr;
  size_t size;
} shmap_t;

void shmap_init(int nprocs);
void shmap_init_child(int id);

#endif /* end of include guard: SHMAP_H */
