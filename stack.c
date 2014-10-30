#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include "safe.h"
#include "util.h"
#include "debug.h"
#include "shmap.h"
#include "stack.h"
#include "config.h"

void *  _stack_addr;
size_t  _stack_size;
void ** _stack_addrs;
int  *  _stack_files;

static void find_stack(void ** addr, size_t * size)
{
  FILE * maps;
  SAFE_RETURN(maps, fopen("/proc/self/maps", "r"));

  void * start;
  void * end;
  char path[FILENAME_LIMIT];

  while (EOF != fscanf(maps,
        "%p-%p %*s %*x %*d:%*d %*d %[[/]%s%*[ (deleted)]\n",
        &start, &end, path, path + 1)) {
    if (strstr(path, "[stack]")) {
      *addr = start;
      *size = end - start;

      DEBUG_PRINTI("stack: addr=%p, size=%ld\n", *addr, *size);
      break;
    }
  }

  SAFE_FNCALL(fclose(maps));
}

void stack_init(int nprocs)
{
  find_stack(&_stack_addr, &_stack_size);

  _stack_addrs = malloc(sizeof(void *) * nprocs);
  _stack_files = malloc(sizeof(int) * nprocs);

  void * stack = stack_new(NULL);
  STACK_EXECUTE(stack,
      _stack_files[0] = shmap_copy(_stack_addr, _stack_size, "stack_0")
  );
  free(stack);

  _stack_addrs[0] = shmap_mmap(NULL, _stack_size, _stack_files[0]);

  SAFE_ASSERT(_stack_addrs[0] < _stack_addr);

  char path[FILENAME_LIMIT];
  int i;

  for (i = 1; i < nprocs; ++i) {
    sprintf(path, "%s_%d", "stack", i);
    _stack_files[i] = shmap_open(_stack_size, path);
    _stack_addrs[i] = shmap_mmap(NULL, _stack_size, _stack_files[i]);

    SAFE_ASSERT(_stack_addrs[i] < _stack_addr);
  }
}

void stack_init_child(int id)
{
  SAFE_ASSERT(id != 0);
  shmap_mmap(_stack_addr, _stack_size, _stack_files[id]);
}

