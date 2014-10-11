#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include "conf.h"
#include "page.h"
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "shmap.h"

#define FNAME_PREFIX "/fibril"
#define FNAME_LENGTH (sizeof(FNAME_PREFIX) + sizeof(size_t) * 2)

static char __data_start, _end;

static int shmap_create(size_t sz)
{
  static size_t fnameSuffix = 0;
  static char   fnamePrefix[] = FNAME_PREFIX;
  char filename[FNAME_LENGTH];

  int i;
  size_t len = sizeof(fnamePrefix) - 1;
  for (i = 0; i < len; ++i) {
    filename[i] = fnamePrefix[i];
  }

  int ret = sprintf(filename + len, "%lx", sync_fadd(&fnameSuffix, 1));
  SAFE_ASSERT(len + ret < FNAME_LENGTH);

  int shm;
  SAFE_RETURN(shm, shm_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_FNCALL(ftruncate(shm, sz));

  DEBUG_PRINT_INFO("created shm: %s fd=%d size=%ld\n", filename, shm, sz);
  return shm;
}

static void load_stack_attr(void ** addr, size_t * size)
{
  FILE * maps;
  SAFE_RETURN(maps, fopen("/proc/self/maps", "r"));

  void * start;
  void * end;
  char path[FILENAME_LIMIT];

  while (EOF != fscanf(maps, "%p-%p %*s %*x %*d:%*d %*d %[[/]%s\n",
        &start, &end, path, path + 1)) {
    if (strstr(path, "[stack]")) {
      *addr = start;
      *size = end - start;

      DEBUG_PRINT_INFO("find stack: addr=%p, size=%ld\n", *addr, *size);
      break;
    }
  }

  SAFE_FNCALL(fclose(maps));
}

void * shmap_mmap(void * addr, size_t sz, int shm)
{
  static int prot = PROT_READ | PROT_WRITE;
  int flags;

  if (shm == -1) {
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
  } else {
    flags = MAP_SHARED;
  }

  if (addr) flags |= MAP_FIXED;

  void * ret = (void *) syscall(SYS_mmap, addr, sz, prot, flags, shm, 0);
  SAFE_ASSERT(ret != MAP_FAILED && (!addr || ret == addr));

  return ret;
}

int shmap_expose(void * addr, size_t size)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_create(size);
  void * buf = shmap_mmap(NULL, size, shm);

  memcpy(buf, addr, size);

  SAFE_FNCALL(munmap(buf, size));
  shmap_mmap(addr, size, shm);

  DEBUG_PRINT_INFO("exposed %p ~ %p shm=%d\n", addr, addr + size, shm);
  return shm;
}

static int helper_thread(void * data)
{
  int  * mutex_init = ((int  * *) data)[0];
  int  * mutex_work = ((int  * *) data)[1];
  int  * mutex_done = ((int  * *) data)[2];
  void * addr       = ((void * *) data)[3];
  size_t size       = ((size_t *) data)[4];
  int  * retptr     = ((int  * *) data)[5];

  sync_lock  (mutex_done);
  sync_unlock(mutex_init);
  sync_lock  (mutex_work);

  *retptr = shmap_expose(addr, size);

  sync_unlock(mutex_done);
  return 0;
}

static int share_main_stack(void * addr, size_t size)
{
  static int mutex_init = 0, mutex_work = 0, mutex_done = 0;

  /** Create child stack. */
  void * stack = shmap_mmap(NULL, size, -1);

  sync_lock(&mutex_init);
  sync_lock(&mutex_work);

  int shm, tid;
  void * data[] = {
    &mutex_init, &mutex_work, &mutex_done, addr, (void *) size, &shm
  };

  SAFE_RETURN(tid, clone(helper_thread, stack + size,
        CLONE_FS | CLONE_FILES | CLONE_VM | SIGCHLD, data));

  /** Tell helper thread to work. */
  sync_unlock(&mutex_work);

  /** Wait until helper thread is initialized. */
  sync_lock(&mutex_init);

  /** Wait until helper thread is done. */
  sync_lock(&mutex_done);

  /** Cleanup */
  int status;
  SAFE_FNCALL(waitpid(tid, &status, 0));
  SAFE_ASSERT(WIFEXITED(status) && 0 == WEXITSTATUS(status));
  SAFE_FNCALL(munmap(stack, size));

  sync_unlock(&mutex_init);
  sync_unlock(&mutex_work);
  sync_unlock(&mutex_done);
  return shm;
}

void shmap_init(int nprocs, shmap_t * stacks[])
{
  /** Share global data segment. */
  shmap_expose(
      PAGE_ALIGN_DOWN(&__data_start),
      PAGE_ALIGN_UP(&_end) - PAGE_ALIGN_DOWN(&__data_start)
  );

  void * stack_addr;
  size_t stack_size;

  load_stack_attr(&stack_addr, &stack_size);

  stacks[0] = malloc(sizeof(shmap_t));
  stacks[0]->fd = share_main_stack(stack_addr, stack_size);
  stacks[0]->addr = shmap_mmap(NULL, stack_size, stacks[0]->fd);
  stacks[0]->size = stack_size;

  int i;
  for (i = 1; i < nprocs; ++i) {
    stacks[i] = malloc(sizeof(shmap_t));
    stacks[i]->fd = shmap_create(stack_size);
    stacks[i]->addr = shmap_mmap(NULL, stack_size, stacks[i]->fd);
    stacks[i]->size = stack_size;
  }
}

/**
 * Overwrite mmap calls.
 */
void * mmap(void * addr, size_t sz, int prot, int flags, int fd, off_t off)
{
  /** Change writable private mapping to shared mapping. */
  void * addr_old = addr;
  int prot_old = prot;
  int flags_old = flags;
  int fd_old = fd;

  if ((prot & PROT_WRITE) && (flags & MAP_PRIVATE)) {
    flags ^= MAP_PRIVATE;
    flags |= MAP_SHARED;

    if (flags & MAP_ANONYMOUS) {
      flags ^= MAP_ANONYMOUS;
      SAFE_ASSERT(fd == -1);
      fd = shmap_create(sz);
    }
  }

  void * ret = (void *) syscall(SYS_mmap, addr, sz, prot, flags, fd, off);

  DEBUG_PRINT_VERBOSE(
      "mmap: addr=%p(%p) sz=%ld prot=0x%x(0x%x) flags=0x%x(0x%x) fd=%d(%d) \
off=%ld ret=%p\n",
      addr, addr_old, sz, prot, prot_old,
      flags, flags_old, fd, fd_old, off, ret);

  return ret;
}

