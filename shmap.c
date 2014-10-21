#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "conf.h"
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "fibrili.h"

typedef struct _shmap_t {
  struct _shmap_t * next;
  void * addr;
  size_t size;
  union {
    int   sh;
    int * tl;
  } file;
  int    otid;
  int    lock;
} shmap_t __attribute ((aligned (sizeof(void *))));

static int _nprocs;
static struct sigaction _default_sa;

static shmap_t * find(shmap_t * head, void * addr)
{
  static shmap_t dummy = { 0 };

  shmap_t * map = head ? head : &dummy;
  shmap_t * next;

  while ((next = map->next) && next->addr <= addr) {
    map = next;
  }

  return map;
}

static void create(int file, void * addr, size_t size)
{
  static shmap_t maps[NUM_SHMAP_ENTRIES];
  static size_t next = 0;

  shmap_t * prev = NULL;
  shmap_t * curr;

  /** Find the mapping before the current one. */
  do {
    prev = find(prev, addr);
    sync_lock(&prev->lock);

    if (prev->next && prev->next->addr <= addr) {
      sync_unlock(&prev->lock);
    } else break;
  } while (1);

  if (prev->addr == addr && prev->size == size) {
    /** Replace the previous map. */
    if (prev->otid == _tid) {
      prev->file.sh = file;
    }

    /** Fill thread local map. */
    else if (prev->otid == -1) {
      prev->file.tl[_tid] = file;
    }

    /** Make a thread local mapping. */
    else {
      int * files = malloc(sizeof(int [_nprocs]));
      files[prev->otid] = prev->file.sh;
      files[_tid] = file;

      prev->file.tl = files;
      prev->otid = -1;
    }

    curr = prev;
  } else {
    curr = &maps[sync_fadd(&next, 1)];

    if (prev->addr == addr && size < prev->size) {
      /** Replace the first part of the previous map. */
      curr->file.sh = prev->file.sh;
      curr->addr = addr + size;
      curr->size = prev->size - size;

      prev->file.sh = file;
      prev->addr = addr;
      prev->size = size;
    } else {
      curr->file.sh = file;
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
}

static void * shmap(void * addr, size_t size, int file)
{
  static int prot = PROT_READ | PROT_WRITE;
  int flag = MAP_SHARED;

  if (NULL != addr) {
    flag |= MAP_FIXED;
  }

  addr = (void *) syscall(SYS_mmap, addr, size, prot, flag, file, 0);
  SAFE_ASSERT(addr != MAP_FAILED);

  return addr;
}

static void segfault_sa(int signum, siginfo_t * info, void * ctxt)
{
  SAFE_ASSERT(signum == SIGSEGV);

  void * addr = info->si_addr;
  shmap_t * map = find(NULL, addr);

  if (map->addr > addr || map->addr + map->size <= addr) {
    sigaction(SIGSEGV, &_default_sa, NULL);
    raise(signum);
  } else {
    SAFE_ASSERT(map->otid != -1);
    shmap(map->addr, map->size, map->file.sh);
  }
}

void shmap_init(int nprocs)
{
  /** Setup SIGSEGV signal handler. */
  struct sigaction sa;
  sa.sa_sigaction = segfault_sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &sa, &_default_sa);
}

int shmap_open(size_t size, const char * name)
{
  static size_t suffix = 0;
  static char prefix[] = "fibril";

  char path[FILENAME_LIMIT];

  if (name) {
    sprintf(path, "/%s.%d.%s" , prefix, _pid, name);
  } else {
    sprintf(path, "/%s.%d.%ld", prefix, _pid, sync_fadd(&suffix, 1));
  }

  int file;
  SAFE_RETURN(file, shm_open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_FNCALL(ftruncate(file, size));

  DEBUG_PRINT_INFO("shmap_open: path=%s file=%d size=%ld\n", path, file, size);
  return file;
}

void * shmap_mmap(void * addr, size_t size, int file)
{
  addr = shmap(addr, size, file);
  create(file, addr, size);
  DEBUG_PRINT_INFO("shmap_mmap: file=%d addr=%p size=%ld\n", file, addr, size);

  return addr;
}

int shmap_copy(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_open(size, name);
  void * buf = shmap(NULL, size, shm);

  memcpy(buf, addr, size);
  SAFE_FNCALL(munmap(buf, size));
  shmap_mmap(addr, size, shm);

  return shm;
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
      file = shmap_open(size, NULL);
    }
  }

  addr = (void *) syscall(SYS_mmap, addr, size, prot, flag, file, off);
  if (addr != MAP_FAILED) create(file, addr, size);

  DEBUG_PRINT_VERBOSE("mmap: file=%d addr=%p size=%ld\n", file, addr, size);
  return addr;
}

