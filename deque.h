#ifndef DEQUE_H
#define DEQUE_H

typedef struct _deque_t {
  struct _deque_t ** link;
  int     lock;
  void ** head;
  void ** tail;
  void * buff[0];
} deque_t;

void deque_init(int nprocs, deque_t ** dqptr);
void deque_init_thread(int id, deque_t ** dqptr);

#endif /* end of include guard: DEQUE_H */
