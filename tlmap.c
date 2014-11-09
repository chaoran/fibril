#include <limits.h>
#include <unistd.h>
#include "page.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "shmap.h"
#include "tlmap.h"

int __fibril_local__ TID;
int __fibril_local__ PID;

intptr_t * TLS_OFFSETS;
char _fibril_tls_start, _fibril_tls_end;

static void * _tls_addr;
static size_t _tls_size;
static int * _tls_files;

extern int _nprocs;

void tlmap_init()
{
  int nprocs = _nprocs;
  void * addr = &_fibril_tls_start;
  size_t size = &_fibril_tls_end - (char *) addr;

  DEBUG_DUMP(2, "shmap_init:", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr));
  DEBUG_ASSERT(PAGE_DIVISIBLE(size));

  DEBUG_ASSERT((void *) &TID >= addr && (void *) &TID < addr + size);
  DEBUG_ASSERT((void *) &PID >= addr && (void *) &PID < addr + size);

  int * files;
  SAFE_NZCALL(files = malloc(sizeof(int [nprocs])));
  files[0] = shmap_copy(addr, size, "tls_0");

  int i;
  for (i = 1; i < nprocs; ++i) {
    char name[NAME_MAX];
    snprintf(name, NAME_MAX, "tls_%d", i);

    files[i] = shmap_open(size, name);
  }

  SAFE_NZCALL(TLS_OFFSETS = malloc(sizeof(intptr_t [nprocs])));

  void * ptr;
  for (i = 0; i < nprocs; ++i) {
    SAFE_NNCALL(ptr = shmap_mmap(NULL, size, files[i]));

    TLS_OFFSETS[i] = ptr - addr;

    /** Initialize tids. */
    int * tid = (void *) &TID + TLS_OFFSETS[i];
    *tid = i;
  }

  PID = getpid();
  _tls_addr = addr;
  _tls_size = size;
  _tls_files = files;
}

void tlmap_init_local(int id)
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

void tlmap_free()
{
  free(TLS_OFFSETS);
}

