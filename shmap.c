#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "tls.h"
#include "conf.h"
#include "page.h"
#include "safe.h"
#include "sync.h"
#include "debug.h"
#include "stack.h"

typedef struct _shmap_t {
  struct _shmap_t * next;
  int    lock;
  int    file;
  void * addr;
  size_t size;
} shmap_t __attribute ((aligned (sizeof(void *))));

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

void shmap_add(int file, void * addr, size_t size)
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
}

int shmap_fix(void * addr)
{
  shmap_t * map = shmap_find(NULL, addr);
  SAFE_ASSERT(map->addr <= addr);

  return (map->addr + map->size > addr);
}

void * shmap_map(void * addr, size_t size, int file)
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
  shmap_add(file, addr, size);

  DEBUG_PRINT_INFO("shmap_map: file=%d addr=%p size=%ld\n", file, addr, size);
  return addr;
}

int shmap_expose(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_new(size, name);
  void * buf = shmap_map(NULL, size, shm);

  memcpy(buf, addr, size);
  shmap_map(addr, size, shm);
  SAFE_FNCALL(munmap(buf, size));

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

  /** Setup stacks. */
  stack_init(nprocs, &TLS.stack);
}

void shmap_init_child(int id)
{
  /** Only child threads need to do this. */
  if (id == 0) return;

  shmap_map(TLS.stack.addr, TLS.stack.size, TLS.stack.maps[id].file);
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
  if (addr != MAP_FAILED) shmap_add(file, addr, size);

  DEBUG_PRINT_VERBOSE("mmap: file=%d addr=%p size=%ld\n", file, addr, size);
  return addr;
}

