#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>

#include "page.h"
#include "safe.h"
#include "stack.h"
#include "debug.h"
#include "fibril.h"
#include "globals.h"

#define MAX_PROCS 256
#define STACK_SIZE 8 * 1024 * 1024

static int _pids[MAX_PROCS];
static int _nproc;
static int _globals;

int __thread FIBRIL_TID;

#ifdef ENABLE_SAFE
static void * _stackaddr;
#endif

static int fibril_main(void * id_)
{
  int id = (int) (size_t) id_;
  FIBRIL_TID = id;

  SAFE_ASSERT(stack_addr() == _stackaddr);
  SAFE_FNCALL(page_map(_globals, GLOBALS_ALIGNED_ADDR, GLOBALS_ALIGNED_SIZE));

  return 0;
}

int fibril_init(int nproc)
{
  int i;

  _globals = page_expose(GLOBALS_ALIGNED_ADDR, GLOBALS_ALIGNED_SIZE);

#ifdef ENABLE_SAFE
  _stackaddr = stack_addr();
#endif

  DEBUG_PRINT_INFO("__data_start=%p, _end=%p, _stackaddr=%p\n",
      &__data_start, &_end, _stackaddr);

  _pids[0] = getpid();

  for (i = 1; i < nproc; ++i) {
    void * stack = malloc(STACK_SIZE);
    void * stackTop = stack + STACK_SIZE;

    SAFE_RETURN(_pids[i], clone(fibril_main, stackTop,
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD,
          (void *) (intptr_t) i));
  }

  return FIBRIL_SUCCESS;
}

int fibril_exit()
{
  int i;

  for (i = 1; i < _nproc; ++i) {
    waitpid(_pids[i], NULL, 0);
  }

  return FIBRIL_SUCCESS;
}

