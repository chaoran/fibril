#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include "safe.h"
#include "debug.h"
#include "sched.h"
#include "stack.h"
#include "shmap.h"
#include "tlmap.h"

int _nprocs;

static int get_nprocs()
{
  int nprocs;
  const char * nprocs_str = getenv("FIBRIL_NPROCS");

  if (nprocs_str) {
    nprocs = atoi(nprocs_str);
  } else {
    nprocs = sysconf(_SC_NPROCESSORS_ONLN);
  }

  return nprocs;
}

static int _main(void * id_)
{
  int id = (int) (size_t) id_;

  tlmap_init_local(id);
  stack_init_local(id);

  if (id) sched_work(id);
  return 0;
}

__attribute__((constructor)) void init()
{
  int nprocs = _nprocs = get_nprocs();
  DEBUG_DUMP(2, "init:", (nprocs, "%d"));

  shmap_init();
  tlmap_init();
  stack_init();
  sched_init();

  int i;
  int flags = CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD;

  for (i = 1; i < nprocs; ++i) {
    void * id = (void *) (intptr_t) i;
    SAFE_NNCALL(clone(_main, STACK_ADDRS[i], flags, id));
  }

  _main(NULL);
}

