#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include "conf.h"
#include "page.h"
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "stack.h"
#include "shmap.h"
#include "fibril.h"
#include "fibrili.h"

int _pids[MAX_PROCS];
char __data_start, _end;
tls_t _tls;

static int _nprocs;
static int _barrier;
static int _tlss[MAX_PROCS];

static void globe_init(int nprocs)
{
  void * dat_start = PAGE_ALIGN_DOWN(&__data_start);
  void * dat_end   = PAGE_ALIGN_UP(&_end);

  DEBUG_PRINT_INFO("data: %p ~ %p\n", dat_start, dat_end);
  SAFE_ASSERT(PAGE_ALIGNED(dat_start) && PAGE_ALIGNED(dat_end));

  void * tls_start = &_tls;
  void * tls_end   = tls_start + sizeof(_tls);

  DEBUG_PRINT_INFO("tls: %p ~ %p\n", tls_start, tls_end);
  SAFE_ASSERT(PAGE_ALIGNED(tls_start) && PAGE_ALIGNED(tls_end));

  if (dat_start < tls_start) {
    shmap_copy(dat_start, tls_start - dat_start, "data");
  }

  if (tls_end < dat_end) {
    shmap_copy(tls_end, dat_end - tls_end, "data_2");
  }
}

static void tls_init(int id)
{
  char path[FILENAME_LIMIT];
  sprintf(path, "tls_%d", id);

  void * addr = &_tls;
  size_t size = sizeof(_tls);

  int file = shmap_copy(addr, size, path);
  _tlss[id] = file;

  sync_barrier(&_barrier, _nprocs);

  tls_t ** addrs = malloc(sizeof(tls_t * [_nprocs]));

  int i;
  for (i = 0; i < _nprocs; ++i) {
    if (i != id) {
      addrs[i] = shmap_mmap(NULL, size, _tlss[i]);
    }
  }

  _tls.x.tls = addrs;
}

static int child_main(void * id_)
{
  int id = (int) (intptr_t) id_;

  _tid = id;
  _pid = getpid();

  tls_init(id);
  stack_init_child(id);
  return 0;
}

int fibril_init(int nprocs)
{
  _tid = 0;
  _pid = getpid();
  _nprocs = nprocs;

  globe_init(nprocs);
  stack_init(nprocs);

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    SAFE_RETURN(_pids[i], clone(child_main, stack_top(i),
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD,
          (void *) (intptr_t) i));
    DEBUG_PRINT_INFO("clone: tid=%d pid=%d stack_top=%p\n",
        i, _pids[i], stack_top(i));
  }

  tls_init(0);
  return FIBRIL_SUCCESS;
}

