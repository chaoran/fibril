#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "page.h"
#include "safe.h"
#include "stack.h"
#include "debug.h"
#include "vtmem.h"
#include "fibril.h"

#define MAX_PROCS 256
#define STACK_SIZE 8 * 1024 * 1024

static pid_t _tids[MAX_PROCS];
static int _nprocs;

int __thread FIBRIL_TID;

static void mem_exercise(int id)
{
  long int i, j, m, n;
  void * s[100];
  struct drand48_data buffer;

  SAFE_ASSERT(0 == srand48_r(id, &buffer));

  for (i = 0; i < 10000; ++i) {
    SAFE_ASSERT(0 == lrand48_r(&buffer, &m));

    for (j = 0; j < m % 100; ++j) {
      SAFE_ASSERT(0 == lrand48_r(&buffer, &n));
      s[j] = malloc(n % 1024);
    }

    for (j = 0; j < m % 100; ++j) {
      free(s[j]);
    }
  }
}

static int fibril_main(int id)
{
  FIBRIL_TID = id;
  DEBUG_PRINT_INFO("initialized: tid=%d\n", _tids[id]);

  mem_exercise(id);

  if (id) {
    exit(EXIT_SUCCESS);
  } else {
    return 0;
  }
}

int fibril_init(int nprocs)
{
  /** Initialize globals. */
  _nprocs = nprocs;
  _tids[0] = syscall(SYS_gettid);

  /** Share global data segment. */
  vtmem_init();

  /** Create workers. */
  int i;
  for (i = 1; i < nprocs; ++i) {
    pid_t pid;
    SAFE_RETURN(pid, syscall(SYS_clone,
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD, NULL));

    if (pid) {
      _tids[i] = pid;
      DEBUG_PRINT_INFO("child process created: id=%d, tid=%d\n", i, _tids[i]);
    } else {
      break;
    }
  }

  fibril_main(i % nprocs);
  return FIBRIL_SUCCESS;
}

int fibril_exit()
{
  int i;

  for (i = 1; i < _nprocs; ++i) {
    int status;
    SAFE_FNCALL(waitpid(_tids[i], &status, 0));
    SAFE_ASSERT(WIFEXITED(status));
  }

  return FIBRIL_SUCCESS;
}

