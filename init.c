#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>

#include "debug.h"
#include "safe.h"
#include "stack.h"
#include "fibril.h"

/** Start and end of global section.*/
/*extern char _etext, _end;*/

/*#define GLOBALS_START PAGE_ALIGN_DOWN(&_etext);*/
/*#define GLOBALS_END   PAGE_ALIGN_UP  (&_end);*/

#define MAX_PROCS 256
#define STACK_SIZE 8 * 1024 * 1024

static int _pids[256];
static int _nproc;

int __thread FIBRIL_TID;

#ifdef ENABLE_SAFE
static void * _stackaddr;
#endif

static int fibril_main(void * id_)
{
  int id = (int) (size_t) id_;
  FIBRIL_TID = id;

  _pids[id] = getpid();

  DEBUG_PRINT_INFO("pid = %d, stackaddr = %p\n", _pids[id], stack_addr());
  SAFE_ASSERT(stack_addr() == _stackaddr);

  return 0;
}

/*static void share_globals()*/
/*{*/
/*}*/

static void spawn_workers(int nproc)
{
  int i;

#ifdef ENABLE_SAFE
  _stackaddr = stack_addr();
#endif

  _pids[0] = getpid();

  for (i = 1; i < nproc; ++i) {
    void * stack = malloc(STACK_SIZE);
    void * stackTop = stack + STACK_SIZE;

    SAFE_FNCALL(clone(fibril_main, stackTop,
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD,
          (void *) (intptr_t) i));
  }
}

int fibril_init(int nproc)
{
  spawn_workers(nproc);

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

