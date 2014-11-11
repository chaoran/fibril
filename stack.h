#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "debug.h"

void   stack_init(int nprocs);
void   stack_init_local(int id, int nprocs);
void   stack_finalize(int nprocs);

extern void * STACK_BOTTOM;
extern size_t STACK_SIZE;
extern void ** STACK_ADDRS;
extern ptrdiff_t * STACK_OFFSETS;

#define STACK_EXECUTE(stack, fcall) do { \
  __asm__ ( "mov\t%0,%%r15\n\txchg\t%%r15,%%rsp" : : "g" (stack) : "r15"); \
  fcall; \
  __asm__ ( "xchg\t%%r15,%%rsp" : ); \
} while (0)

#endif /* end of include guard: STACK_H */
