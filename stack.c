#define _GNU_SOURCE
#include <pthread.h>
#include "debug.h"

void * stack_addr()
{
  pthread_attr_t attr;
  pthread_getattr_np(pthread_self(), &attr);

  void * addr;
  size_t size;

  pthread_attr_getstack(&attr, &addr, &size);

  return addr;
}
