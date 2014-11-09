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

void *  STACK_ADDR;
void *  STACK_BOTTOM;
void ** STACK_ADDRS;
intptr_t * STACK_OFFSETS;

static int  *_files;
extern int _nprocs;

static void find_stack(void ** addr, size_t * size)
{
  FILE * maps;
  SAFE_NNCALL(maps = fopen("/proc/self/maps", "r"));

  void * start;
  void * end;
  char path[FILENAME_LIMIT];

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

void stack_init()
{
  int nprocs = _nprocs;

  size_t size;
  find_stack(&STACK_ADDR, &size);

  STACK_BOTTOM = STACK_ADDR + size;

  SAFE_NZCALL(_files = malloc(sizeof(int) * nprocs));
  SAFE_NZCALL(STACK_OFFSETS = malloc(sizeof(intptr_t) * nprocs));

  void * stack;
  SAFE_NZCALL(stack = malloc(size));
  stack += size;

  STACK_EXECUTE(stack, _files[0] = shmap_copy(STACK_ADDR, size, "stack_0"));

  free(stack - size);

  void * addrs[nprocs];
  addrs[0] = shmap_mmap(NULL, size, _files[0]);

  char path[FILENAME_LIMIT];
  int i;

  for (i = 1; i < nprocs; ++i) {
    sprintf(path, "%s_%d", "stack", i);

    _files[i] = shmap_open(size, path);
    addrs[i] = shmap_mmap(NULL, size, _files[i]);
  }

  for (i = 0; i < nprocs; ++i) {
    DEBUG_DUMP(2, "stack_init:", (i, "%d"), (addrs[i], "%p"));
    STACK_OFFSETS[i] = addrs[i] - STACK_ADDR;
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
    shmap_mmap(STACK_ADDR, STACK_BOTTOM - STACK_ADDR, _files[id]);
    barrier();
  } else {
    barrier();
    free(_files);
  }
}

void stack_free()
{
  int i;
  int nprocs = _nprocs;
  size_t size = STACK_BOTTOM - STACK_ADDR;

  for (i = 0; i < nprocs; ++i) {
    free(STACK_ADDRS[i] - size);
  }

  free(STACK_ADDRS);
  free(STACK_OFFSETS);
}

