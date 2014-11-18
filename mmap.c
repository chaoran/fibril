#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include "page.h"
#include "safe.h"
#include "util.h"
#include "shmap.h"
#include "tlmap.h"
#include "config.h"

typedef struct _map_t {
  struct _map_t * next;
  void * addr;
  size_t size;
  int prot;
  int flag;
  int file;
  off_t off;
} map_t;

#define MAX_MAPS 1024

static int _next;
static void * _addr;
static size_t _size;
static map_t __fibril_local__ _maps[MAX_MAPS];
static map_t __fibril_local__ _base;
static map_t * __fibril_local__ _mapped;
static map_t * __fibril_local__ _unmapped;
static map_t * __fibril_local__ _unused;
static struct sigaction _default_sa;

extern int _nprocs;

static int find_owner(void * addr)
{
  if (addr < _addr || addr >= _addr + _size) return -1;

  size_t size = (size_t) PAGE_ALIGN_DOWN(_size / _nprocs);

  int i = (addr - _addr) / size;
  int id = (_nprocs - i - 1);

  DEBUG_ASSERT(id >= 0 && id < _nprocs);
  return id;
}

static void segfault(int signum, siginfo_t * info, void * ctxt)
{
  DEBUG_ASSERT(signum == SIGSEGV);

  void * addr = info->si_addr;

  /** Found out who owns the mapping. */
  int id = find_owner(addr);
  DEBUG_ASSERT(id != TID);

  if (id != -1) {
    const map_t * map = *(map_t **) tlmap_shptr(&_mapped, id);

    /** Find the mapping. */
    while (map) {
      if (addr >= map->addr && addr < map->addr + map->size) {
        DEBUG_DUMP(1, "segfault:", (addr, "%p"), (map->addr, "%p"),
            (map->size, "%ld"), (map->file, "%d"));
        SAFE_NNCALL(syscall(SYS_mmap, map->addr, map->size, map->prot,
              map->flag, map->file, map->off));
        return;
      }

      map = map->next;
    }
  }

  DEBUG_BREAK(1);
  sigaction(SIGSEGV, &_default_sa, NULL);
  raise(signum);
}

__attribute__((constructor)) void init()
{
  const char * mapping = "%p-%p %*s %*x %*d:%*d %*d %*[[/]%*s%*[ (deleted)]\n";
  const void * limit = (void *) 0x7ffffffff000;

  FILE * file = fopen("/proc/self/maps", "r");
  SAFE_ASSERT(file != NULL);

  void * prev_start, * prev_end;

  SAFE_NNCALL(fscanf(file, mapping, &prev_start, &prev_end));

  void * start, * end;

  _size = 0;

  while (EOF != fscanf(file, mapping, &start, &end) && start < limit) {
    if (start - prev_end > _size) {
      _addr = prev_end;
      _size = start - prev_end;
    }

    prev_start = start;
    prev_end   = end;
  }

  SAFE_NNCALL(fclose(file));

  DEBUG_DUMP(2, "mmap_init:", (_addr, "%p"), (_size, "%lu"));

  _base.addr = _addr;
  _base.size = _size;
  _unmapped = &_base;

  /** Setup segfault handler. */
  struct sigaction sa;
  sa.sa_sigaction = segfault;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &sa, &_default_sa);
}

static map_t * new_map()
{
  map_t * map;

  if (_unused) {
    map = _unused;
    _unused = map->next;
  } else {
    SAFE_ASSERT(_next < MAX_MAPS);

    map = &_maps[_next++];

    /** Use the global pointer if tls is initialized. */
    if (TLS_OFFSETS) {
      map = tlmap_shptr(map, TID);
    }
  }

  return map;
}

static map_t * rewrite(map_t * map)
{
  DEBUG_ASSERT(TID == 0);

  extern void * _fibril_tls_start, * _fibril_tls_end;

  if (map == NULL) return NULL;

  map->next = rewrite(map->next);

  void * start = &_fibril_tls_start;
  void *   end = &_fibril_tls_end;
  void *  addr = (void *) map;

  if (addr >= start && addr < end) {
    addr = tlmap_shptr(addr, 0);
  }

  return addr;
}

void mmap_init_local(int id, int nprocs)
{
  DEBUG_ASSERT(TLS_OFFSETS != NULL);

  _base.size = (size_t) PAGE_ALIGN_DOWN(_size / nprocs);
  _base.addr = _addr + (nprocs - id - 1) * _base.size;

  if (id) {
    _unmapped = &_base;
  } else {
    /** Rewrite pointers to use global pointers on mapped entries. */
    _unmapped = rewrite(_unmapped);
    _mapped = rewrite(_mapped);
    _unused = rewrite(_unused);
  }

  DEBUG_DUMP(2, "mmap_init_local:", (_base.addr, "%p"), (_base.size, "%lu"));
}

static void free_map(map_t * map)
{
  map->next = _unused;
  _unused = map;
}

/**
 * Allocate an virtual address space.
 */
static void * alloc(size_t size)
{
  DEBUG_ASSERT(PAGE_ALIGNED(size));

  /** Find a unmmapped region that is large enough. */
  map_t * curr = _unmapped;
  map_t * prev = NULL;

  while (curr && curr->size < size) {
    prev = curr;
    curr = curr->next;
  }

  DEBUG_ASSERT(curr != NULL && curr->size >= size);

  /** Cut out a piece from the unmapped region. */
  void * addr = curr->addr;
  curr->addr += size;
  curr->size -= size;

  /** Remove the region from _unmapped if it's size becomes zero. */
  if (curr->size == 0) {
    if (prev) {
      prev->next = curr->next;
    } else {
      _unmapped = curr->next;
    }

    free_map(curr);
  }

  return addr;
}

static int remove_map(void * addr, size_t size)
{
  /**
   * @todo: Need to unmap from everyone that has mapped the same region.
   */
  if (TID != find_owner(addr)) return 0;

  map_t * prev = NULL;
  map_t * curr = _mapped;

  while (curr) {
    if (addr >= curr->addr && addr < curr->addr + curr->size) {
      DEBUG_ASSERT(addr + size <= curr->addr + curr->size);

      /** Remove the mapped entry if unmapping the whole map. */
      if (addr == curr->addr && size == curr->size) {
        if (prev) {
          prev->next = curr->next;
        } else {
          _mapped = curr->next;
        }

        free_map(curr);
      }
      /** Cut the top. */
      else if (addr == curr->addr) {
        curr->addr += size;
        curr->size -= size;
        curr->off += size;
      }
      /** Cut the bottom. */
      else if (addr + size == curr->addr + curr->size) {
        curr->size -= size;
      }
      /** Cut from the middle. */
      else {
        curr->size = addr - curr->addr;

        map_t * map = new_map();
        map->next = curr->next;
        map->addr = addr + size;
        map->size = curr->addr + curr->size - map->addr;
        map->prot = curr->prot;
        map->flag = curr->flag;
        map->file = curr->file;
        map->off  = curr->off + (map->addr - curr->addr);

        curr->next = map;
      }

      break;
    }

    prev = curr;
    curr = curr->next;
  }

  return (curr != NULL);
}

static void coalesce(map_t * left, map_t * right)
{
  if (!left || !right || left->addr + left->size != right->addr) return;

  left->size += right->size;
  left->next = right->next;

  free_map(right);
}

int munmap(void * addr, size_t size)
{
  if (remove_map(addr, size)) {
    map_t * prev = NULL;
    map_t * next = _unmapped;

    while (next && next->addr < addr) {
      prev = next;
      next = next->next;
    }

    if (!next) {
      _unmapped = new_map();
      _unmapped->next = NULL;
      _unmapped->addr = addr;
      _unmapped->size = size;
    }
    else if (next->addr == addr + size) {
      next->addr -= size;
      next->size += size;
      coalesce(prev, next);
    }
    else if (prev && prev->addr + prev->size == addr) {
      prev->size += size;
      coalesce(prev, next);
    }
    else {
      map_t * map = new_map();
      map->addr = addr;
      map->size = size;
      map->next = next;

      if (prev) {
        prev->next = map;
      } else {
        _unmapped = map;
      }
    }
  }

  return syscall(SYS_munmap, addr, size);
}

void * mmap(void * addr, size_t size, int prot, int flag, int file, off_t off)
{
  if (prot & PROT_WRITE) {
    if (flag & MAP_PRIVATE) {
      if (flag & MAP_ANONYMOUS || file == -1) {
        flag ^= MAP_PRIVATE;
        flag |= MAP_SHARED;
        flag &= ~MAP_ANONYMOUS;
        file = shmap_open(size, NULL);
      }
    }
  }

  if (addr == NULL && (addr = alloc(size))) {
    flag |= MAP_FIXED;
  }

  /** Register the mmap. */
  map_t * map = new_map();
  map->next = _mapped;
  map->addr = addr;
  map->size = size;
  map->prot = prot;
  map->flag = flag;
  map->file = file;
  map->off  = off;

  _mapped = map;

  DEBUG_DUMP(3, "mmap:", (addr, "%p"), (size, "%lu"), (file, "%d"));
  return (void *) syscall(SYS_mmap, addr, size, prot, flag, file, off);
}

