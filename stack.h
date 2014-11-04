#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void   stack_init(int nprocs);
void   stack_init_child(int id);

extern void *  _stack_addr;
extern size_t  _stack_size;
extern void *  _stack_bottom;
extern void ** _stack_addrs;
extern int  *  _stack_files;

#define STACK_FREE_INPLACE 1
#define STACK_FREE_ATTOP 0

static inline
void * stack_new(void * frame)
{
  size_t sz;
  void * addr;

  if (frame) {
    SAFE_ASSERT(frame >= _stack_addr && frame < _stack_addr + _stack_size);
    sz = _stack_addr + _stack_size - frame;
    addr = malloc(sz);
    DEBUG_PRINTV("stack_new: addr=%p size=%ld\n", addr, sz);
  } else {
    sz = _stack_size;
    addr = malloc(sz) + _stack_size;
  }

  return addr;
}

static inline
void stack_free(void * addr, int inplace)
{
  if (inplace) {
    free(addr);
  } else {
    free(addr - _stack_size);
  }
}

static inline
void * stack_shptr(void * ptr, int id)
{
  SAFE_ASSERT(ptr >= _stack_addr && ptr <= _stack_bottom);
  return _stack_addrs[id] + (ptr - _stack_addr);
}

#define STACK_EXECUTE(stack, fcall) do { \
  __asm__ ( "mov\t%0,%%r15\n\txchg\t%%r15,%%rsp" : : "g" (stack) : "r15"); \
  fcall; \
  __asm__ ( "xchg\t%%r15,%%rsp" : ); \
} while (0)

#endif /* end of include guard: STACK_H */
