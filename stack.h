#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include "shmap.h"

void stack_range(void ** addr, size_t * size);
void * stack_top(int id);
shmap_t * stack_copy(void * addr, size_t size);

#endif /* end of include guard: STACK_H */
