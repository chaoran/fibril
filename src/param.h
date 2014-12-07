#ifndef PARAM_H
#define PARAM_H

#include <stddef.h>

extern size_t PARAM_PAGE_SIZE;
extern void * PARAM_STACK_ADDR;
extern size_t PARAM_STACK_SIZE;
extern int PARAM_NUM_PROCS;

#define PAGE_ALIGN_DOWN(x) ((void *) ((size_t) (x) & ~(PARAM_PAGE_SIZE - 1)))
#define PAGE_ALIGNED(x) (0 == ((size_t) (x) & (PARAM_PAGE_SIZE - 1)))

#endif /* end of include guard: PARAM_H */
