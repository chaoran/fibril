#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include "tls.h"
#include "page.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "sched.h"
#include "stack.h"
#include "shmap.h"
#include "fibrili.h"

char __data_start, _end;

int     _nprocs;
int  *  _pids;

static void globe_init(int nprocs)
{
  shmap_init(nprocs);

  void * dat_start = PAGE_ALIGN_DOWN(&__data_start);
  void * dat_end   = PAGE_ALIGN_UP(&_end);

  DEBUG_DUMP(2, "data", (dat_start, "%p"), (dat_end, "%p"));
  DEBUG_ASSERT(PAGE_ALIGNED(dat_start) && PAGE_ALIGNED(dat_end));

  shmap_copy(dat_start, dat_end - dat_start, "data");
}

static int _main(void * id_)
{
  int id = (int) (size_t) id_;

  tls_init_local(id);
  stack_init_local(id);

  barrier();

  if (id) sched_work(id, _nprocs);
  return 0;
}

int fibril_init(int nprocs)
{
  _nprocs = nprocs;
  _pids = malloc(sizeof(int) * nprocs);

  globe_init(nprocs);
  tls_init(nprocs);
  stack_init(nprocs);
  sched_init(nprocs);

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    SAFE_NNCALL(
        _pids[i] = clone(_main, STACK_ADDRS[i],
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD, (void *) (size_t) i)
    );
  }

  _main((void *) 0);
  return 0;
}

