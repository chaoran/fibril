#ifndef FIBRILI_H
#define FIBRILI_H

struct _fibril_t {
  int count;
  char lock;
  void * stack;
  void * retval;
  struct {
    void * rsp;
    void * rbp;
    void * rbx;
    void * r12;
    void * r13;
    void * r14;
    void * r15;
    void * rip;
  } regs;
};

extern __thread struct _fibrili_deque_t {
  char lock;
  int  head;
  int  tail;
  void * buff[1000];
} fibrili_deq;

#define fibrili_fence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define fibrili_lock(l) while (__atomic_test_and_set(&(l), __ATOMIC_ACQUIRE))
#define fibrili_unlock(l) __atomic_clear(&(l), __ATOMIC_RELEASE)

#include "setjmp.h"

extern int fibrili_join(struct _fibril_t * frptr);
__attribute__((noreturn)) extern void fibrili_resume(struct _fibril_t * frptr);
__attribute__((noreturn)) extern void fibrili_yield(struct _fibril_t * frptr);

extern inline
void fibrili_push(struct _fibril_t * frptr)
{
  fibrili_deq.buff[fibrili_deq.tail++] = frptr;
}

extern inline
int fibrili_pop(void)
{
  int tail = fibrili_deq.tail;

  if (tail == 0) return 0;

  fibrili_deq.tail = --tail;

  fibrili_fence();

  if (fibrili_deq.head > tail) {
    fibrili_deq.tail = tail + 1;

    fibrili_lock(fibrili_deq.lock);

    if (fibrili_deq.head > tail) {
      fibrili_deq.head = 0;
      fibrili_deq.tail = 0;

      fibrili_unlock(fibrili_deq.lock);
      return 0;
    }

    fibrili_deq.tail = tail;
    fibrili_unlock(fibrili_deq.lock);
  }

  return 1;
}

__attribute__((always_inline)) extern inline
int fibrili_join(struct _fibril_t * frptr)
{
  fibrili_lock(frptr->lock);

  int success = (frptr->count-- == 0);

  if (success) {
    fibrili_unlock(frptr->lock);
  }

  return success;
}

#endif /* end of include guard: FIBRILI_H */
