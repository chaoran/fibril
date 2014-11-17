#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "page.h"
#include "safe.h"
#include "util.h"
#include "tlmap.h"
#include "config.h"
#include "fibrili.h"

char __data_start, _end;
char _fibril_shm_start, _fibril_shm_end;

void * shmap_mmap(void * addr, size_t size, int file)
{
  static int prot = PROT_READ | PROT_WRITE;
  int flag = MAP_SHARED;

  if (NULL != addr) {
    flag |= MAP_FIXED;
  }

  addr = (void *) syscall(SYS_mmap, addr, size, prot, flag, file, 0);
  DEBUG_ASSERT(addr != MAP_FAILED);

  DEBUG_DUMP(3, "shmap_mmap:", (addr, "%p"), (size, "%ld"), (file, "%d"));
  return addr;
}

int shmap_open(size_t size, const char * name)
{
  static size_t suffix = 0;
  static char prefix[] = "fibril";

  char path[FILENAME_LIMIT];

  if (name) {
    sprintf(path, "/%s.%d.%s" , prefix, PID, name);
  } else {
    sprintf(path, "/%s.%d.%ld", prefix, PID, fadd(&suffix, 1));
  }

  int file;
  SAFE_NNCALL(file = shm_open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_NNCALL(ftruncate(file, size));
  SAFE_NNCALL(shm_unlink(path));

  DEBUG_DUMP(3, "shmap_open:", (path, "%s"), (file, "%d"), (size, "%ld"));
  return file;
}

int shmap_copy(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_open(size, name);
  void * buf = shmap_mmap(NULL, size, shm);

  memcpy(buf, addr, size);
  SAFE_NNCALL(munmap(buf, size));
  shmap_mmap(addr, size, shm);

  return shm;
}

void shmap_init(int nprocs)
{
  void * addr = PAGE_ALIGN_DOWN(&__data_start);
  size_t size = PAGE_ALIGN_UP(&_end) - addr;

  DEBUG_DUMP(2, "shmap_init (app):", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr) && PAGE_DIVISIBLE(size));

  shmap_copy(addr, size, "data");

  addr = &_fibril_shm_start;
  size = (void *) &_fibril_shm_end - addr;

  DEBUG_DUMP(2, "shmap_init (lib):", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr) && PAGE_DIVISIBLE(size) && size > 0);

  shmap_copy(addr, size, "shared");
}

