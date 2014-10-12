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

#include "tls.h"
#include "conf.h"
#include "page.h"
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "shmap.h"

#define FNAME_PREFIX "/fibril"

char __data_start, _end;
tls_t _tls;

static int shmap_create(size_t sz, const char * name)
{
  static size_t fnameSuffix = 0;
  static char   fnamePrefix[] = FNAME_PREFIX;
  char filename[FILENAME_LIMIT];

  int i;
  size_t len = sizeof(fnamePrefix) - 1;
  for (i = 0; i < len; ++i) {
    filename[i] = fnamePrefix[i];
  }

  /** Append .[pid]. */
  len += sprintf(filename + len, ".%d", PID);
  SAFE_ASSERT(len < FILENAME_LIMIT);

  /** Append .[name | seqno (if anonymous) ]. */
  if (name) {
    len += sprintf(filename + len, ".%s", name);
  } else {
    len += sprintf(filename + len, ".%ld", sync_fadd(&fnameSuffix, 1));
  }
  SAFE_ASSERT(len < FILENAME_LIMIT);

  int shm;
  SAFE_RETURN(shm, shm_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_FNCALL(ftruncate(shm, sz));

  DEBUG_PRINT_INFO("created shm: filename=%s fd=%d size=%ld\n",
      filename, shm, sz);

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

  DEBUG_PRINT_INFO("mmapped: %p ~ %p fd=%d\n", ret, ret + sz, shm);
  return ret;
}

int shmap_expose(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_create(size, name);
  void * buf = shmap_mmap(NULL, size, shm);

  memcpy(buf, addr, size);
  shmap_mmap(addr, size, shm);

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

  *retptr = shmap_expose(addr, size, "stack_0");

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

void shmap_init(int nprocs)
{
  /** Make sure tls is page aligned and is a multiple pages. */
  SAFE_ASSERT(PAGE_ALIGNED(&_tls));
  SAFE_ASSERT(PAGE_DIVISIBLE(sizeof(_tls)));

  void * data_start = PAGE_ALIGN_DOWN(&__data_start);
  void * data_end   = PAGE_ALIGN_UP(&_end);
  void * tls_start = &_tls;
  void * tls_end   = (void *) &_tls + sizeof(_tls);

  /** Make sure tls is within the data segment. */
  DEBUG_PRINT_INFO("data: %p ~ %p\n", data_start, data_end);
  DEBUG_PRINT_INFO("tls: %p ~ %p\n", tls_start, tls_end);
  SAFE_ASSERT(data_start <= tls_start && tls_end <= data_end);

  /** Share global data segment but exclude tls pages. */
  if (data_start < tls_start) {
    shmap_expose(data_start, tls_start - data_start, "globals");
  }

  if (tls_end < data_end) {
    shmap_expose(tls_end, data_end - tls_end, "globals");
  }

  /** Find out main stack addr and size. */
  load_stack_attr(&TLS.stack_addr, &TLS.stack_size);

  void * stack_addr = TLS.stack_addr;
  size_t stack_size = TLS.stack_size;
  shmap_t * stacks = TLS.stacks;

  /** Allocate stacks for everyone. */
  stacks[0].fd = share_main_stack(stack_addr, stack_size);
  stacks[0].addr = shmap_mmap(NULL, stack_size, stacks[0].fd);
  stacks[0].size = stack_size;

  int i;
  char name[FILENAME_LIMIT];

  for (i = 1; i < nprocs; ++i) {
    int len = sprintf(name, "stack_%d", i);
    SAFE_ASSERT(len < FILENAME_LIMIT);

    stacks[i].fd = shmap_create(stack_size, name);
    stacks[i].addr = shmap_mmap(NULL, stack_size, stacks[i].fd);
    stacks[i].size = stack_size;
  }
}

void shmap_init_child(int id)
{
  /** Only child threads need to do this. */
  if (id == 0) return;

  void * addr = shmap_mmap(TLS.stack_addr, TLS.stack_size, TLS.stacks[id].fd);
  SAFE_ASSERT(addr == TLS.stack_addr);
}

/**
 * Overwrite mmap calls.
 */
void * mmap(void * addr, size_t sz, int prot, int flags, int fd, off_t off)
{
  /** Change writable private mapping to shared mapping. */
  if ((prot & PROT_WRITE) && (flags & MAP_PRIVATE)) {
    flags ^= MAP_PRIVATE;
    flags |= MAP_SHARED;

    if (flags & MAP_ANONYMOUS) {
      flags ^= MAP_ANONYMOUS;
      SAFE_ASSERT(fd == -1);
      fd = shmap_create(sz, NULL);
    }
  }

  addr = (void *) syscall(SYS_mmap, addr, sz, prot, flags, fd, off);

  DEBUG_PRINT_VERBOSE("mmap: %p ~ %p fd=%d off=%ld\n",
      addr, addr + sz, fd, off);

  return addr;
}

