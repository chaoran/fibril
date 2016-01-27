#include "mutex.h"
#include "sync.h"

#define NULL ((void *) 0)
#define spin_wait(x) while (!(x)) __asm__ ( "pause" ::: "memory" )

void mutex_lock(mutex_t ** mutex, mutex_t * node)
{
  node->next = NULL;
  mutex_t * prev = sync_swap(mutex, node);

  if (prev) {
    node->flag = 0;
    prev->next = node;
    spin_wait(node->flag);
  }
}

int mutex_trylock(mutex_t ** mutex, mutex_t * node)
{
  node->next = NULL;
  mutex_t * prev = sync_cas(mutex, NULL, node);
  return (prev == NULL);
}

void mutex_unlock(mutex_t ** mutex, mutex_t * node)
{
  if (node->next == NULL) {
    if (node == sync_cas(mutex, node, NULL)) {
      return;
    }

    spin_wait(node->next);
  }

  node->next->flag = MUTEX_LOCKED;
}

