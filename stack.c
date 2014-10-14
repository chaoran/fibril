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
#include "shmap.h"

static void load_stack_attr(void ** addr, size_t * size)
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

      DEBUG_PRINT_INFO("find stack: addr=%p, size=%ld\n", *addr, *size);
      break;
    }
  }

  SAFE_FNCALL(fclose(maps));
}

static int helper_thread(void * data)
{
  int  * mutex_init = ((int  * *) data)[0];
  int  * mutex_work = ((int  * *) data)[1];
  int  * mutex_done = ((int  * *) data)[2];
  void * addr       = ((void * *) data)[3];
  size_t size       = ((size_t *) data)[4];
  int  * retptr     = ((int  * *) data)[5];

  sync_lock  (mutex_done);
  sync_unlock(mutex_init);
  sync_lock  (mutex_work);

  *retptr = shmap_expose(addr, size, "stack_0");

  sync_unlock(mutex_done);
  return 0;
}

static int share_main_stack(void * addr, size_t size)
{
  static int mutex_init = 0, mutex_work = 0, mutex_done = 0;

  /** Create child stack. */
  void * stack = shmap_map(NULL, size, -1);

  sync_lock(&mutex_init);
  sync_lock(&mutex_work);

  int shm, tid;
  void * data[] = {
    &mutex_init, &mutex_work, &mutex_done, addr, (void *) size, &shm
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
  SAFE_FNCALL(munmap(stack, size));

  sync_unlock(&mutex_init);
  sync_unlock(&mutex_work);
  sync_unlock(&mutex_done);
  return shm;
}

void stack_init(int nprocs, stack_info_t * info)
{
  /** Find out main stack addr and size. */
  void * stack_addr;
  size_t stack_size;

  load_stack_attr(&stack_addr, &stack_size);

  struct stack_map * maps = malloc(sizeof(struct stack_map [nprocs]));

  /** Allocate stacks for everyone. */
  maps[0].file = share_main_stack(stack_addr, stack_size);
  maps[0].addr = shmap_map(NULL, stack_size, maps[0].file);

  int i;
  char name[FILENAME_LIMIT];

  for (i = 1; i < nprocs; ++i) {
    int len = sprintf(name, "stack_%d", i);
    SAFE_ASSERT(len < FILENAME_LIMIT);

    maps[i].file = shmap_new(stack_size, name);
    maps[i].addr = shmap_map(NULL, stack_size, maps[i].file);
  }

  info->addr = stack_addr;
  info->size = stack_size;
  info->maps = maps;
}

