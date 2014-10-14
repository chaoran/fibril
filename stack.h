#ifndef STACK_H
#define STACK_H

#include <stddef.h>

struct stack_map {
  int    file;
  void * addr;
};

typedef struct {
  void * addr;
  size_t size;
  struct stack_map * maps;
} stack_info_t;

void stack_init(int nprocs, stack_info_t * info);

#endif /* end of include guard: STACK_H */
