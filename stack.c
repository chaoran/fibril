#include <stdio.h>
#include <limits.h>
#include "safe.h"
#include "util.h"
#include "shmap.h"
#include "stack.h"
#include "tlmap.h"

void * STACK_BOTTOM;
size_t STACK_SIZE;
void ** STACK_ADDRS;
ptrdiff_t * STACK_OFFSETS;

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

      break;
    }
  }

  SAFE_NNCALL(fclose(maps));
}

void stack_init(int nprocs)
{
  void * addr;
  size_t size;
  find_stack(&addr, &size);

  DEBUG_DUMP(2, "stack_init:", (addr, "%p"), (size, "%ld"));

  STACK_SIZE = size;
  STACK_BOTTOM = addr + size;

  STACK_OFFSETS = tlmap_setup(addr, size, "stack", nprocs);

  /** Allocate space for scheduler stacks. */
  SAFE_NZCALL(STACK_ADDRS = malloc(sizeof(void * [nprocs])));

  int i;
  for (i = 0; i < nprocs; ++i) {
    SAFE_NZCALL(STACK_ADDRS[i] = malloc(size));
    STACK_ADDRS[i] += size;
  }
}

void stack_init_local(int id, int nprocs)
{
  tlmap_setup_local(STACK_OFFSETS, id, nprocs);
}

void stack_finalize(int nprocs)
{
  free(STACK_OFFSETS);

  int i;
  for (i = 0; i < nprocs; ++i) {
    free(STACK_ADDRS[i] - STACK_SIZE);
  }

  free(STACK_ADDRS);
}

