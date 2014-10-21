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
#include "shmap.h"
#include "stack.h"

static void * _stack_addr;
static size_t _stack_size;
static void ** _stack_addrs;
static int  *  _stack_files;

static int helper_thread(void * data)
{
  int  * mutex_init = ((int  * *) data)[0];
  int  * mutex_work = ((int  * *) data)[1];
  int  * mutex_done = ((int  * *) data)[2];
  void * addr       = ((void * *) data)[3];
  size_t size       = ((size_t *) data)[4];
  int  * file       = ((int  * *) data)[5];

  sync_lock  (mutex_done);
  sync_unlock(mutex_init);
  sync_lock  (mutex_work);

  *file = shmap_copy(addr, size, "stack");

  sync_unlock(mutex_done);
  return 0;
}

static int copy_stack(void * addr, size_t size)
{
  static int mutex_init = 0;
  static int mutex_work = 0;
  static int mutex_done = 0;

  /** Create child stack.*/
  void * stack = malloc(size);

  sync_lock(&mutex_init);
  sync_lock(&mutex_work);

  int tid, file;
  void * data[] = {
    &mutex_init, &mutex_work, &mutex_done, addr, (void *)size, &file
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

  return file;
}

static void find_stack(void ** addr, size_t * size)
{
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
}

void stack_init(int nprocs)
{
  find_stack(&_stack_addr, &_stack_size);

  _stack_addrs = malloc(sizeof(void * [nprocs]));
  _stack_files = malloc(sizeof(int [nprocs]));

  _stack_files[0] = copy_stack(_stack_addr, _stack_size);
  _stack_addrs[0] = shmap_mmap(NULL, _stack_size, _stack_files[0]);

  char path[FILENAME_LIMIT];
  int i;

  for (i = 1; i < nprocs; ++i) {
    sprintf(path, "%s_%d", "stack", i);
    _stack_files[i] = shmap_open(_stack_size, path);
    _stack_addrs[i] = shmap_mmap(NULL, _stack_size, _stack_files[i]);
  }
}

void stack_init_child(int id)
{
  SAFE_ASSERT(id != 0);
  shmap_mmap(_stack_addr, _stack_size, _stack_files[id]);
}

void * stack_top(int id)
{
  return _stack_addrs[id] + _stack_size;
}

