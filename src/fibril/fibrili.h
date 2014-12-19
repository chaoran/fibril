#ifndef FIBRILI_H
#define FIBRILI_H

typedef struct _fibrili_state_t {
  void * rbp;
  void * rbx;
  void * r12;
  void * r13;
  void * r14;
  void * r15;
  void * rip;
} * fibrili_state_t;

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

__attribute__((hot)) static inline
void fibrili_push(struct _fibril_t * frptr)
{
  fibrili_deq.buff[fibrili_deq.tail++] = frptr;
}

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

__attribute__((noinline, hot)) static
int fibrili_setjmp(fibrili_state_t state)
{
  register int ret asm ("eax");
  void * rip = __builtin_return_address(0);

  __asm__ ( "mov\t%%rbp,0x0(%1)\n\t"
            "mov\t%%rbx,0x8(%1)\n\t"
            "mov\t%%r12,0x10(%1)\n\t"
            "mov\t%%r13,0x18(%1)\n\t"
            "mov\t%%r14,0x20(%1)\n\t"
            "mov\t%%r15,0x28(%1)\n\t"
            "mov\t%2,0x30(%1)\n\t"
            "xor\t%0,%0\n\t"
            : "=r" (ret), "+r" (state) : "r" (rip) );
  return ret;
}

#endif /* end of include guard: FIBRILI_H */
