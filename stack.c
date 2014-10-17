#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "stack.h"

static void * _stack_addr;
static size_t _stack_size;

shmap_t ** _stack_maps;

static int helper_thread(void * data)
{
  int  * mutex_init = ((int  * *) data)[0];
  int  * mutex_work = ((int  * *) data)[1];
  int  * mutex_done = ((int  * *) data)[2];
  void * addr       = ((void * *) data)[3];
  size_t size       = ((size_t *) data)[4];
  shmap_t ** map    = ((shmap_t ***) data)[5];

  sync_lock  (mutex_done);
  sync_unlock(mutex_init);
  sync_lock  (mutex_work);

  *map = shmap_copy(addr, size, "stack");

  sync_unlock(mutex_done);
  return 0;
}

shmap_t * stack_copy(void * addr, size_t size)
{
  static int mutex_init = 0;
  static int mutex_work = 0;
  static int mutex_done = 0;

  /** Create child stack.*/
  void * stack = malloc(size);

  sync_lock(&mutex_init);
  sync_lock(&mutex_work);

  int tid;
  shmap_t * map;

  void * data[] = {
    &mutex_init, &mutex_work, &mutex_done, addr, (void *)size, &map
  };

  SAFE_RETURN(tid, clone(helper_thread, stack + size,
        CLONE_FS | CLONE_FILES | CLONE_VM | SIGCHLD, data));

  /** Tell helper thread to work. */
  sync_unlock(&mutex_work);

  /** Wait until helper thread is initialized. */
  sync_lock(&mutex_init);

  /** Wait until helper thread is done. */
  sync_lock(&mutex_done);

  /** Cleanup */
  int status;
  SAFE_FNCALL(waitpid(tid, &status, 0));
  SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  free(stack);

  sync_unlock(&mutex_init);
  sync_unlock(&mutex_work);
  sync_unlock(&mutex_done);

  return map;
}

void * stack_top(int id)
{
  return _stack_maps[id]->addr + _stack_maps[id]->size;
}

void stack_range(void ** addr, size_t * size)
{
  if (_stack_addr && _stack_size) {
    *addr = _stack_addr;
    *size = _stack_size;

    return;
  }

  FILE * maps;
  SAFE_RETURN(maps, fopen("/proc/self/maps", "r"));

  void * start;
  void * end;
  char path[FILENAME_LIMIT];

  while (EOF != fscanf(maps, "%p-%p %*s %*x %*d:%*d %*d %[[/]%s\n",
        &start, &end, path, path + 1)) {
    if (strstr(path, "[stack]")) {
      *addr = start;
      *size = end - start;

      DEBUG_PRINT_INFO("stack: addr=%p, size=%ld\n", *addr, *size);
      break;
    }
  }

  SAFE_FNCALL(fclose(maps));

  _stack_addr = *addr;
  _stack_size = *size;
}

