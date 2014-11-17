#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include "safe.h"
#include "util.h"
#include "sched.h"
#include "stack.h"
#include "shmap.h"
#include "tlmap.h"

int   _nprocs;
int * __fibril_shared__ _pids;

static int _main(void * id_)
{
  int id = (int) (size_t) id_;

  tlmap_init_local(id, _nprocs);
  stack_init_local(id, _nprocs);

  barrier();

  if (id) sched_work(id, _nprocs);
  return 0;
}

int fibril_init(int nprocs)
{
  _nprocs = nprocs;
  _pids = malloc(sizeof(int) * nprocs);

  shmap_init(nprocs);
  tlmap_init(nprocs);
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

