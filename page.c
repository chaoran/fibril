#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "page.h"
#include "safe.h"
#include "debug.h"
#include "atomic.h"

#define FNAME_PREFIX "/fibril"
#define FNAME_SEQNO_BASE 16
#define FNAME_LENGTH (sizeof(FNAME_PREFIX) + sizeof(size_t) * 2)

static void create_unique_filename(char * filename)
{
  static char fnamePrefix[] = FNAME_PREFIX;
  static size_t fnameSuffix = 0;

  int i;
  size_t size = sizeof(fnamePrefix) - 1;
  for (i = 0; i < size; ++i) {
    filename[i] = fnamePrefix[i];
  }

  sprintf(filename + size, "%lx", atomic_fadd(&fnameSuffix, 1));
}

int page_expose(void * addr, size_t size)
{
  char filename[FNAME_LENGTH];
  create_unique_filename(filename);

  int shm;
  SAFE_RETURN(shm, shm_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_FNCALL(ftruncate(shm, size));

  /** Map the shm to a temporary address to copy the content of the pages. */
  void * buf;

  SAFE_RETURN(buf, mmap(NULL, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0));
  memcpy(buf, addr, size);

  SAFE_FNCALL(munmap(buf, size));

  SAFE_RETURN(buf, mmap(addr, size,
        PROT_READ  | PROT_WRITE,
        MAP_SHARED | MAP_FIXED, shm, 0));
  SAFE_ASSERT(buf == addr);

  DEBUG_PRINT_INFO("created shared mapping \"%s\" at %p ~ %p\n",
      filename, addr, addr + size);
  return shm;
}

void * page_map(int fd, void * addr, size_t size)
{
  SAFE_ASSERT(PAGE_ALIGNED(addr));
  SAFE_ASSERT(PAGE_DIVISIBLE(size));

  return mmap(addr, size, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_FIXED, fd, 0);
}
