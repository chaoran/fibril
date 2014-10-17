#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "conf.h"
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "shmap.h"

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

static shmap_t * create(int file, void * addr, size_t size)
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
    if (prev->istl == 0) {
      prev->file.sh = file;
    }

    /** Fill thread local map. */
    else {
      prev->file.tl[FIBRILi_TID] = file;
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
  return curr;
}

int shmap_open(size_t size, const char * name)
{
  static size_t suffix = 0;
  static char prefix[] = "/fibril";

  char path[FILENAME_LIMIT];
  size_t len = sizeof(prefix) - 1;

  strncpy(path, prefix, len);

  /** Append .[pid]. */
  len += sprintf(path + len, ".%d", FIBRILi_PID);
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

  DEBUG_PRINT_INFO("shmap_open: path=%s file=%d size=%ld\n", path, file, size);
  return file;
}

shmap_t * shmap_find(void * addr)
{
  shmap_t * map = find(NULL, addr);

  if (addr < map->addr || addr >= map->addr + map->size) {
    return NULL;
  }

  return map;
}

shmap_t * shmap_mmap(void * addr, size_t size, int file)
{
  int prot = PROT_READ | PROT_WRITE;
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
  SAFE_ASSERT(addr != MAP_FAILED);

  DEBUG_PRINT_INFO("shmap_mmap: file=%d addr=%p size=%ld\n", file, addr, size);
  return create(file, addr, size);
}

shmap_t * shmap_copy(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_open(size, name);
  void * buf = shmap_mmap(NULL, size, shm)->addr;

  memcpy(buf, addr, size);
  SAFE_FNCALL(munmap(buf, size));
  return shmap_mmap(addr, size, shm);
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

