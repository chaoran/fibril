#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include "safe.h"
#include "util.h"
#include "debug.h"
#include "shmap.h"
#include "stack.h"

void *  STACK_ADDR;
void *  STACK_BOTTOM;
void ** STACK_ADDRS;
intptr_t * STACK_OFFSETS;

static int  *  _stack_files;

static void find_stack(void ** addr, size_t * size)
{
  FILE * maps = fopen("/proc/self/maps", "r");
  SAFE_ASSERT(maps != NULL);

  void * start;
  void * end;
  char path[PATH_MAX];

  while (EOF != fscanf(maps,
        "%p-%p %*s %*x %*d:%*d %*d %[[/]%s%*[ (deleted)]\n",
        &start, &end, path, path + 1)) {
    if (strstr(path, "[stack]")) {
      *addr = start;
      *size = end - start;

      DEBUG_DUMP(2, "stack:", (start, "%p"), (end, "%p"));
      break;
    }
  }

  SAFE_NNCALL(fclose(maps));
}

void stack_init(int nprocs)
{
  size_t size;
  find_stack(&STACK_ADDR, &size);

  STACK_BOTTOM = STACK_ADDR + size;

  SAFE_NZCALL(_stack_files = malloc(sizeof(int) * nprocs));

  void * stack = malloc(size);
  SAFE_ASSERT(stack != NULL);

  stack += size;

  STACK_EXECUTE(stack,
      _stack_files[0] = shmap_copy(STACK_ADDR, size, "stack_0")
  );

  free(stack - size);

  int i;
  char name[NAME_MAX];

  SAFE_NZCALL(STACK_OFFSETS = malloc(sizeof(intptr_t) * nprocs));
  STACK_OFFSETS[0] = shmap_mmap(NULL, size, _stack_files[0]) - STACK_ADDR;

  for (i = 1; i < nprocs; ++i) {
    SAFE_NNCALL(snprintf(name, NAME_MAX, "%s_%d", "stack", i));

    _stack_files[i] = shmap_open(size, name);
    STACK_OFFSETS[i] = shmap_mmap(NULL, size, _stack_files[i]) - STACK_ADDR;
  }

  /** Allocate space for scheduler stacks. */
  SAFE_NZCALL(STACK_ADDRS = malloc(sizeof(void * [nprocs])));

  for (i = 0; i < nprocs; ++i) {
    SAFE_NZCALL(STACK_ADDRS[i] = malloc(size));
    STACK_ADDRS[i] += size;
  }
}

void stack_init_local(int id)
{
  if (id) {
    shmap_mmap(STACK_ADDR, STACK_BOTTOM - STACK_ADDR, _stack_files[id]);
    barrier();
  }
  else {
    barrier();
    free(_stack_files);
  }
}

void stack_finalize(int nprocs)
{
  free(STACK_OFFSETS);

  int i;
  for (i = 0; i < nprocs; ++i) {
    free(STACK_ADDRS[i] - (STACK_BOTTOM - STACK_ADDR));
  }

  free(STACK_ADDRS);
}

