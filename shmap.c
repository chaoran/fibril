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

char __data_start, _end;
tls_t _tls;

int shmap_new(size_t size, const char * name)
{
  static size_t suffix = 0;
  static char prefix[] = "/fibril";

  char path[FILENAME_LIMIT];

  int i;
  size_t len = sizeof(prefix) - 1;

  for (i = 0; i < len; ++i) {
    path[i] = prefix[i];
  }

  /** Append .[pid]. */
  len += sprintf(path + len, ".%d", PID);
  SAFE_ASSERT(len < FILENAME_LIMIT);

  /** Append .[name | seqno (if anonymous) ]. */
  if (name) {
    len += sprintf(path + len, ".%s", name);
  } else {
    len += sprintf(path + len, ".%ld", sync_fadd(&suffix, 1));
  }
  SAFE_ASSERT(len < FILENAME_LIMIT);

  int file;
  SAFE_RETURN(file, shm_open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_FNCALL(ftruncate(file, size));

  DEBUG_PRINT_INFO("shmap_new: path=%s file=%d size=%ld\n", path, file, size);
  return file;
}

static shmap_t * shmap_find(shmap_t * head, void * addr)
{
  static shmap_t dummy = { 0 };

  shmap_t * map = head ? head : &dummy;
  shmap_t * next;

  while ((next = map->next) && next->addr <= addr) {
    map = next;
  }

  return map;
}

shmap_t * shmap_add(int file, void * addr, size_t size)
{
  static shmap_t maps[NUM_SHMAP_ENTRIES];
  static size_t next = 0;

  shmap_t * prev = NULL;
  shmap_t * curr;

  /** Find the mapping before the current one. */
  do {
    prev = shmap_find(prev, addr);
    sync_lock(&prev->lock);

    if (prev->next && prev->next->addr <= addr) {
      sync_unlock(&prev->lock);
    } else break;
  } while (1);

  if (prev->addr == addr && prev->size == size) {
    /** Replace the previous map. */
    prev->file = file;
    curr = prev;
  } else {
    curr = &maps[sync_fadd(&next, 1)];

    if (prev->addr == addr && size < prev->size) {
      /** Replace the first part of the previous map. */
      curr->file = prev->file;
      curr->addr = addr + size;
      curr->size = prev->size - size;

      prev->file = file;
      prev->addr = addr;
      prev->size = size;
    } else {
      curr->file = file;
      curr->addr = addr;
      curr->size = size;

      /** Replace the last part of the previous map. */
      if (addr < prev->addr + prev->size) {
        prev->size = addr - prev->addr;
      }
    }

    curr->next = prev->next;
    prev->next = curr;
  }

  sync_unlock(&prev->lock);
  return curr;
}

int shmap_fix(void * addr)
{
  shmap_t * map = shmap_find(NULL, addr);
  SAFE_ASSERT(map->addr <= addr);

  return (map->addr + map->size > addr);
}

shmap_t * shmap_map(int file, void * addr, size_t size)
{
  static int prot = PROT_READ | PROT_WRITE;

  int flag;

  if (-1 == file) {
    flag = MAP_PRIVATE | MAP_ANONYMOUS;
  } else {
    flag = MAP_SHARED;
  }

  if (NULL != addr) {
    flag |= MAP_FIXED;
  }

  addr = (void *) syscall(SYS_mmap, addr, size, prot, flag, file, 0);
  SAFE_ASSERT(addr != MAP_FAILED && PAGE_DIVISIBLE((size_t) addr));

  DEBUG_PRINT_INFO("shmap_map: file=%d addr=%p size=%ld\n", file, addr, size);
  return shmap_add(file, addr, size);
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

int shmap_expose(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_new(size, name);
  void * buf = shmap_map(shm, NULL, size)->addr;

  memcpy(buf, addr, size);
  shmap_map(shm, addr, size);
  SAFE_FNCALL(munmap(buf, size));

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
  void * stack = shmap_map(-1, NULL, size)->addr;

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
  shmap_t ** stacks = TLS.stacks;

  /** Allocate stacks for everyone. */
  int fd = share_main_stack(stack_addr, stack_size);
  stacks[0] = shmap_map(fd, NULL, stack_size);

  int i;
  char name[FILENAME_LIMIT];

  for (i = 1; i < nprocs; ++i) {
    int len = sprintf(name, "stack_%d", i);
    SAFE_ASSERT(len < FILENAME_LIMIT);

    int fd = shmap_new(stack_size, name);
    stacks[i] = shmap_map(fd, NULL, stack_size);
  }
}

void shmap_init_child(int id)
{
  /** Only child threads need to do this. */
  if (id == 0) return;

  shmap_map(TLS.stacks[id]->file, TLS.stack_addr, TLS.stack_size);
}

/**
 * Overwrite mmap calls.
 */
void * mmap(void * addr, size_t size, int prot, int flag, int file, off_t off)
{
  if ((prot & PROT_WRITE) && (flag & MAP_PRIVATE)) {
    /** Change private to shared. */
    flag ^= MAP_PRIVATE;
    flag |= MAP_SHARED;

    /** Change anonymous to file-backed. */
    if (flag & MAP_ANONYMOUS) {
      SAFE_ASSERT(file == -1);
      flag ^= MAP_ANONYMOUS;
      file = shmap_new(size, NULL);
    }
  }

  addr = (void *) syscall(SYS_mmap, addr, size, prot, flag, file, off);
  shmap_add(file, addr, size);

  DEBUG_PRINT_VERBOSE("mmap: file=%d addr=%p size=%ld\n", file, addr, size);
  return addr;
}

