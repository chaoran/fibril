#ifndef SHMAP_H
#define SHMAP_H

#include <stddef.h>

typedef struct _shmap_t {
  struct _shmap_t * next;
  void * addr;
  size_t size;
  union {
    int   sh;
    int * tl;
  } file;
  int    istl;
  int    lock;
} shmap_t __attribute ((aligned (sizeof(void *))));

int       shmap_open(size_t size, const char * name);
shmap_t * shmap_mmap(void * addr, size_t size, int file);
shmap_t * shmap_find(void * addr);
shmap_t * shmap_copy(void * addr, size_t size, const char * name);
void   shmap_mmap_tl(void * addr, size_t size, int id, int file);

#define shmap_mktl(map, nprocs) do { \
  int * files = malloc(sizeof(int [nprocs])); \
  files[0] = map->file.sh; \
  map->file.tl = files; \
  map->istl = 1; \
} while (0)

#endif /* end of include guard: SHMAP_H */
