#include <stdlib.h>

#include "conf.h"
#include "page.h"
#include "deque.h"
#include "debug.h"

void deque_init(int nprocs, deque_t ** dqptr)
{
  deque_t ** link = malloc(sizeof(deque_t *) * nprocs);

  int i;
  for (i = 0; i < nprocs; ++i) {
    deque_t * deq = malloc(NUM_DEQUE_PAGES * PAGE_SIZE);
    deq->lock = 0;
    deq->head = deq->buff;
    deq->tail = deq->buff;
    deq->link = link;

    link[i] = deq;
  }

  DEBUG_PRINT_INFO("deque: %p\n", link[0]);
  *dqptr = link[0];
}

void deque_init_thread(int id, deque_t ** dqptr)
{
  if (id == 0) return;

  deque_t * deq = *dqptr;
  deq = deq->link[id];

  DEBUG_PRINT_INFO("deque: %p\n", deq);
  *dqptr = deq;
}

