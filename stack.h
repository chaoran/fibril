#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void   stack_init(int nprocs);
void   stack_init_child(int id);
void   stack_finalize(int nprocs);

extern void * STACK_ADDR;
extern void * STACK_BOTTOM;
extern int  *  STACK_FILES;
extern void ** STACK_ADDRS;
extern intptr_t * STACK_OFFSETS;

static inline
void * stack_shptr(void * ptr, int id)
{
  SAFE_ASSERT(ptr >= STACK_ADDR && ptr <= STACK_BOTTOM);
  return ptr + STACK_OFFSETS[id];
}

#define STACK_EXECUTE(stack, fcall) do { \
  __asm__ ( "mov\t%0,%%r15\n\txchg\t%%r15,%%rsp" : : "g" (stack) : "r15"); \
  fcall; \
  __asm__ ( "xchg\t%%r15,%%rsp" : ); \
} while (0)

#endif /* end of include guard: STACK_H */
