#ifndef FIBRILI_H
#define FIBRILI_H

struct _fibril_t {
  char lock;
  char unmapped;
  int count;
  struct {
    void * btm;
    void * top;
    void * ptr;
  } stack;
  void * pc;
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

__attribute__((noinline)) extern
void fibrili_join(struct _fibril_t * frptr);
__attribute__((noreturn)) extern
void fibrili_resume(struct _fibril_t * frptr);

#define fibrili_push(frptr) do { \
  (frptr)->pc = __builtin_return_address(0); \
  fibrili_deq.buff[fibrili_deq.tail++] = (frptr); \
} while (0)

__attribute__((hot)) static
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

#define fibrili_membar(call) do { \
  call; \
  __asm__ ( "nop" : : : "rbx", "r12", "r13", "r14", "r15", "memory" ); \
} while (0)

#endif /* end of include guard: FIBRILI_H */
