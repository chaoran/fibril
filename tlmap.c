#include <limits.h>
#include <stdlib.h>
#include "safe.h"
#include "page.h"
#include "util.h"
#include "shmap.h"
#include "stack.h"
#include "tlmap.h"

typedef struct _tlmap_t {
  void * addr;
  size_t size;
  int files[0];
} tlmap_t;

int __fibril_local__ TID;
int __fibril_local__ PID;
ptrdiff_t * TLS_OFFSETS;

char _fibril_tls_start, _fibril_tls_end;

ptrdiff_t * tlmap_setup(void * addr, size_t size, const char * name, int nprocs)
{
  /** Only the main thread can call this function. */
  SAFE_ASSERT(TID == 0);

  tlmap_t * map = malloc(sizeof(tlmap_t) + sizeof(int [nprocs]));
  SAFE_ASSERT(map != NULL);

  map->addr = addr;
  map->size = size;

  ptrdiff_t * diffs = malloc(sizeof(ptrdiff_t [nprocs + 1]));
  SAFE_ASSERT(diffs != NULL);

  char filename[NAME_MAX];
  SAFE_NNCALL(snprintf(filename, NAME_MAX, "%s_%d", name, 0));

  register void * rsp asm ("rsp");

  if (rsp >= addr && rsp < addr + size) {
    DEBUG_ASSERT(STACK_SIZE != 0);
    void * stack = malloc(STACK_SIZE) + STACK_SIZE;
    STACK_EXECUTE(stack, map->files[0] = shmap_copy(addr, size, filename));
    free(stack - STACK_SIZE);
  } else {
    map->files[0] = shmap_copy(addr, size, filename);
  }

  int i;
  for (i = 1; i < nprocs; ++i) {
    SAFE_NNCALL(snprintf(filename, NAME_MAX, "%s_%d", name, i));
    map->files[i] = shmap_open(size, filename);
  }

  for (i = 0; i < nprocs; ++i) {
    diffs[i] = shmap_mmap(NULL, size, map->files[i]) - addr;
  }

  /** Put the map as the last element of diffs array. */
  diffs[i] = (ptrdiff_t) map;
  return diffs;
}

void tlmap_setup_local(ptrdiff_t * diffs, int id, int nprocs)
{
  tlmap_t * map = (void *) diffs[nprocs];

  if (id) {
    shmap_mmap(map->addr, map->size, map->files[id]);
    barrier();
  } else {
    barrier();
    free(map);
  }
}

void tlmap_init(int nprocs)
{
  void * addr = &_fibril_tls_start;
  size_t size = (void *) &_fibril_tls_end - addr;

  DEBUG_DUMP(2, "tlmap_init:", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr));
  DEBUG_ASSERT(PAGE_DIVISIBLE(size));

  DEBUG_ASSERT((void *) &TID >= addr && (void *) &TID < addr + size);
  DEBUG_ASSERT((void *) &PID >= addr && (void *) &PID < addr + size);

  TLS_OFFSETS = tlmap_setup(addr, size, "tls", nprocs);
  PID = getpid();

  int i;
  for (i = 1; i < nprocs; ++i) {
    int * ptid = (void *) &TID + TLS_OFFSETS[i];
    *ptid = i;
  }
}

void tlmap_init_local(int id, int nprocs)
{
  tlmap_setup_local(TLS_OFFSETS, id, nprocs);
  if (id != 0) PID = getpid();
}

