#ifndef FIBRILI_H
#define FIBRILI_H

#include "setjmp.h"

struct _fibril_t {
  char lock;
  int count;
  struct {
    void * ptr;
    void * top;
  } stack;
  struct _fibrili_state_t state;
};

extern __thread struct _fibrili_deque_t {
  char lock;
  int  head;
  int  tail;
  void * stack;
  void * buff[1000];
} fibrili_deq;

#define fibrili_fence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define fibrili_lock(l) do { \
  __asm__ ( "pause" : : : "memory" ); \
} while (__atomic_test_and_set(&(l), __ATOMIC_ACQUIRE))
#define fibrili_unlock(l) __atomic_clear(&(l), __ATOMIC_RELEASE)

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

#endif /* end of include guard: FIBRILI_H */
