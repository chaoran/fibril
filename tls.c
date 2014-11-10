#include <limits.h>
#include <stdlib.h>
#include "tls.h"
#include "page.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "shmap.h"

int __fibril_local__ TID;
int __fibril_local__ PID;

intptr_t * TLS_OFFSETS;
char _fibril_tls_start, _fibril_tls_end;

static void * _tls_addr;
static size_t _tls_size;
static int * _tls_files;

void tls_init(int nprocs)
{
  void * addr = &_fibril_tls_start;
  size_t size = (void *) &_fibril_tls_end - addr;

  DEBUG_DUMP(2, "tls_init:", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr));
  DEBUG_ASSERT(PAGE_DIVISIBLE(size));

  DEBUG_ASSERT((void *) &TID >= addr && (void *) &TID < addr + size);
  DEBUG_ASSERT((void *) &PID >= addr && (void *) &PID < addr + size);

  /** Create tls files. */
  int i;
  char name[NAME_MAX];

  int * files = malloc(sizeof(int [nprocs]));
  SAFE_ASSERT(files != NULL);

  files[0] = shmap_copy(addr, size, "tls_0");

  for (i = 1; i < nprocs; ++i) {
    SAFE_NNCALL(snprintf(name, NAME_MAX, "tls_%d", i));
    files[i] = shmap_open(size, name);
  }

  /** Create shared mappings for tls files. */
  TLS_OFFSETS = malloc(sizeof(intptr_t [nprocs]));
  SAFE_ASSERT(TLS_OFFSETS != NULL);

  for (i = 0; i < nprocs; ++i) {
    TLS_OFFSETS[i] = shmap_mmap(NULL, size, files[i]) - addr;
  }

  /** Initialize TIDs. */
  for (i = 0; i < nprocs; ++i) {
    int * ptid = (void *) &TID + TLS_OFFSETS[i];
    *ptid = i;
  }

  /** Initialize PID. */
  PID = getpid();

  _tls_addr  = addr;
  _tls_size  = size;
  _tls_files = files;
}

void tls_init_local(int id)
{
  if (id) {
    shmap_mmap(_tls_addr, _tls_size, _tls_files[id]);
    barrier();

    PID = getpid();
  }
  else {
    barrier();
    free(_tls_files);
  }
}

