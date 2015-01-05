#ifndef STACK_H
#define STACK_H

#include "fibrili.h"

extern __thread size_t time_map_count, time_unmap_count;
extern __thread double time_map_sum, time_unmap_sum;

void * stack_setup(struct _fibril_t * frptr);
void stack_reinstall(struct _fibril_t * frptr);
void stack_uninstall(struct _fibril_t * frptr);

#endif /* end of include guard: STACK_H */
