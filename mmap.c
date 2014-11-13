#include <stdio.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include "page.h"
#include "safe.h"
#include "util.h"
#include "shmap.h"
#include "tlmap.h"
#include "config.h"

typedef struct _bin_t {
  struct _bin_t * next;
  void * addr;
  size_t size;
} bin_t;

static bin_t * __fibril_local__ _head;
static bin_t __fibril_local__ _base;

static int load_map(FILE * maps, void ** start, void ** end)
{
  return fscanf(maps,
      "%p-%p %*s %*x %*d:%*d %*d %*[[/]%*s%*[ (deleted)]\n", start, end);
}

__attribute__((constructor)) void mmap_init(int nprocs)
{
  FILE * maps = fopen("/proc/self/maps", "r");
  SAFE_ASSERT(maps != NULL);

  void * addr;
  size_t size = 0;

  void * prev_start;
  void * prev_end;
  void * start;
  void * end;

  load_map(maps, &prev_start, &prev_end);

  while (EOF != load_map(maps, &start, &end)
      && start < (void *) 0x7ffffffff000) {
    if (start - prev_end > size) {
      addr = prev_end;
      size = start - prev_end;
    }

    prev_start = start;
    prev_end   = end;
  }

  SAFE_NNCALL(fclose(maps));

  DEBUG_DUMP(2, "mmap_init:", (addr, "%p"), (size, "%lu"));

  _base.next = NULL;
  _base.addr = addr;
  _base.size = size;

  _head = &_base;
}

void mmap_init_local(int id, int nprocs)
{
  bin_t * pbase = (void *) &_base + TLS_OFFSETS[0];

  size_t size = (pbase->size / nprocs) & ~PAGE_SIZE_MASK;
  void * addr = pbase->addr + (nprocs - id - 1) * size;

  if (id) {
    _base.addr = addr;
    _base.size = size;
    _head = &_base;
    barrier();
  } else {
    barrier();
    _base.addr = addr;
    _base.size = size;
  }

  DEBUG_DUMP(2, "mmap_init_local:", (_base.addr, "%p"), (_base.size, "%lu"));
}

static void * alloc(size_t size)
{
  DEBUG_ASSERT(PAGE_ALIGNED(size));

  /** Find a bin that is large enough. */
  bin_t * curr = _head;
  bin_t * prev = NULL;

  while (curr && curr->size < size) {
    prev = curr;
    curr = curr->next;
  }

  DEBUG_ASSERT(curr != NULL && curr->size >= size);

  /** Cut out space from the bin. */
  void * addr = curr->addr;
  curr->addr += size;
  curr->size -= size;

  /** Remove the bin if we have cleared the bin. */
  if (curr->size == 0) {
    if (prev) {
      prev->next = curr->next;
    } else {
      _head = curr->next;
    }
  }

  return addr;
}

static int _munmap(void * addr, size_t size)
{
  return syscall(SYS_munmap, addr, size);
}

static size_t _free(void * addr, size_t size)
{
  bin_t * prev = NULL;
  bin_t * curr = NULL;
  bin_t * next = _head;

  while (next && next->addr < addr) {
    prev = curr;
    curr = next;
    next = curr->next;
  }

  if (next == NULL) return size;

  if (next->addr == addr + size) {
    /** Put the space into the next bin. */
    next->addr -= size;
    next->size += size;
  }

  else if (curr && curr->addr + curr->size == addr) {
    /** Put the space into the curr bin. */
    curr->size += size;
  }

  else {
    /** Create a new bin for the space. */
    bin_t * bin = addr + size - PAGE_SIZE;
    bin->next = next;
    bin->addr = addr;
    bin->size = size;

    if (curr) {
      curr->next = bin;
    } else {
      _head = bin;
    }

    return size - PAGE_SIZE;
  }

  /** Coalesce bins. */
  if (curr && curr->addr + curr->size == next->addr) {
    next->addr -= curr->size;
    next->size += curr->size;

    if (prev) {
      prev->next = curr;
    } else {
      _head = next;
    }

    _munmap(curr, PAGE_SIZE);
  }

  return size;
}

void * mmap(void * addr, size_t size, int prot, int flag, int file, off_t off)
{
  if (prot & PROT_WRITE && flag & MAP_PRIVATE) {
    /** @todo Need to handle mapping that is backed by a file. */
    DEBUG_ASSERT(flag & MAP_ANONYMOUS && file == -1);

    flag ^= MAP_PRIVATE;
    flag |= MAP_SHARED;

    flag ^= MAP_ANONYMOUS;
    file = shmap_open(size, NULL);
  }

  DEBUG_ASSERT(file != -1);

  if (addr == NULL) {
    addr = alloc(size);
    off = 0;
  }

  shmap_create(addr, size, file, off);
  DEBUG_DUMP(3, "mmap:", (addr, "%p"), (size, "%lu"), (file, "%d"));
  return (void *) syscall(SYS_mmap, addr, size, prot, flag, file, off);
}

int munmap(void * addr, size_t size)
{
  size = _free(addr, size);
  return _munmap(addr, size);
}

