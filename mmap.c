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

static void * _addr;
static size_t _size;
static struct sigaction _default_sa;
static int __fibril_local__ _next;
static map_t __fibril_local__ _maps[MAX_MAPS];
static map_t * __fibril_local__ _mapped;
static map_t * __fibril_local__ _unmapped;
static map_t * __fibril_local__ _unused;

extern int _nprocs;

static int find_owner(void * addr)
{
  if (addr < _addr || addr >= _addr + _size) return -1;

  size_t size = (size_t) PAGE_ALIGN_DOWN(_size / _nprocs);
  int id = (addr - _addr) / size;

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

  DEBUG_DUMP(2, "mmap_init:", (_addr, "%p"), (_size, "0x%lx"));

  _unmapped = new_map();
  _unmapped->addr = _addr;
  _unmapped->size = _size;
  _unmapped->file = -1;

  /** Setup segfault handler. */
  struct sigaction sa;
  sa.sa_sigaction = segfault;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &sa, &_default_sa);
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

  size_t size = (size_t) PAGE_ALIGN_DOWN(_size / nprocs);
  void * addr = _addr + id * size;

  if (id) {
    _unmapped = new_map();
    _unmapped->addr = addr;
    _unmapped->size = size;
    _unmapped->file = -1;
  } else {
    /** Rewrite pointers to use global pointers on mapped entries. */
    _unmapped = rewrite(_unmapped);
    _mapped = rewrite(_mapped);
    _unused = rewrite(_unused);

    map_t * base = _unmapped;

    while (base->file != -1) {
      base = base->next;
    }

    base->size = addr + size - base->addr;
  }

  DEBUG_DUMP(2, "mmap_init_local:", (addr, "%p"), (size, "0x%lx"));
}

static void free_map(map_t * map)
{
  map->next = _unused;
  _unused = map;
}

static map_t * slice(map_t ** list, void * addr, size_t size)
{
  DEBUG_ASSERT(*list && (*list)->addr <= addr);

  map_t dummy;
  dummy.next = *list;

  map_t * prev = &dummy;
  map_t * curr = prev->next;

  map_t * head;
  map_t * tail;

  while (curr && curr->addr + curr->size <= addr) {
    prev = curr;
    curr = curr->next;
  }

  head = curr;

  map_t * next = curr->next;

  while (next && next->addr < addr + size && addr < next->addr + next->size) {
    curr = next;
    next = next->next;
  }

  tail = curr;

  if (head->addr < addr) {
    size_t diff = addr - head->addr;

    prev = head;

    head = new_map();
    head->next = prev->next;
    head->addr = addr;
    head->size = prev->size - diff;
    head->prot = prev->prot;
    head->flag = prev->flag;
    head->file = prev->file;
    head->off  = prev->off + diff;

    prev->size = diff;

    if (tail == prev) tail = head;
  }

  if (tail->addr + tail->size > addr + size) {
    size_t diff = tail->addr + tail->size - (addr + size);

    next = new_map();
    next->next = tail->next;
    next->addr = addr + size;
    next->size = diff;
    next->prot = tail->prot;
    next->flag = tail->flag;
    next->file = tail->file;
    next->off  = tail->off + (tail->size - diff);

    tail->size -= diff;
    tail->next = next;
  }

  prev->next = tail->next;
  tail->next = NULL;

  *list = dummy.next;
  return head;
}

static map_t * coalesce(map_t * left, map_t * right)
{
  DEBUG_ASSERT(left != NULL);

  /** Coalesce if they are continuous. */
  if (right && left->file == right->file &&
      right->addr == left->addr + left->size) {
    DEBUG_ASSERT(left->off + left->size == right->off);
    left->size += right->size;
    left->next = right->next;
    free_map(right);

    return left;
  }

  /** Do not coalesce. */
  else {
    left->next = right;
    return right;
  }
}

static map_t * insert(map_t * list, map_t * maps)
{
  if (!list) return maps;

  /** Find the last node. */
  map_t * tail = maps;

  while (tail->next) {
    tail = tail->next;
  }

  /** Return maps if the list is after the maps. */
  if (tail->addr + tail->size <= list->addr) {
    coalesce(tail, list);
    return maps;
  }

  /** Find a position in the list to insert. */
  map_t * prev = list;
  map_t * next = list->next;

  while (next && next->addr < tail->addr + tail->size) {
    prev = next;
    next = next->next;
  }

  DEBUG_ASSERT(prev && prev->addr + prev->size <= maps->addr);
  DEBUG_ASSERT(!next || next->addr >= tail->addr + tail->size);

  coalesce(prev, maps);
  coalesce(tail, next);

  return list;
}

int munmap(void * addr, size_t size)
{
  if (TID == find_owner(addr)) {
    map_t * maps = slice(&_mapped, addr, size);
    _unmapped = insert(_unmapped, maps);
  }

  DEBUG_DUMP(3, "munmap:", (addr, "%p"), (size, "0x%lx"));
  return syscall(SYS_munmap, addr, size);
}

/**
 * Allocate an virtual address space.
 */
static map_t * alloc(size_t size, int file)
{
  DEBUG_ASSERT(size > 0);
  DEBUG_ASSERT(PAGE_ALIGNED(size));
  DEBUG_ASSERT(_unmapped != NULL);

  map_t * head = _unmapped;
  map_t * curr = head;

  if (file == -1) {
    /** Find a unmapped consecutive region that is large enough. */
    map_t * next = head->next;
    size_t  totl = curr->size;

    while (totl < size) {
      if (next->addr == curr->addr + curr->size) {
        totl += next->size;
      } else {
        totl = next->size;
        head = next;
      }

      curr = next;
      next = next->next;
    }

    head = slice(&_unmapped, head->addr, size);
  } else {
    while (curr->file != -1) {
      curr = curr->next;
    }

    DEBUG_ASSERT(curr && curr->size >= size);

    head = new_map();
    head->next = NULL;
    head->addr = curr->addr;
    head->size = size;
    head->file = -1;

    curr->addr += size;
    curr->size -= size;
  }

  return head;
}

static inline void * _mmap(const map_t * map)
{
  void * addr = map->addr;
  size_t size = map->size;
  int prot  = map->prot;
  int flag  = map->flag;
  int file  = map->file;
  off_t off = map->off;

  DEBUG_DUMP(3, "mmap:", (addr, "%p"), (size, "0x%lx"), (file, "%d"));
  void * ret = (void *) syscall(SYS_mmap, addr, size, prot, flag, file, off);
  return ret;
}

void * mmap(void * addr, size_t size, int prot, int flag, int file, off_t off)
{
  DEBUG_ASSERT(addr == NULL && (flag & MAP_FIXED) == 0);

  flag |= MAP_FIXED;
  map_t * head = alloc(size, file);

  DEBUG_ASSERT(head != NULL);

  if (prot & PROT_WRITE) {
    if (flag & MAP_PRIVATE) {
      if (flag & MAP_ANONYMOUS || file == -1) {
        flag ^= MAP_PRIVATE;
        flag |= MAP_SHARED;
        flag &= ~MAP_ANONYMOUS;
      }
    }
  }

  void * ret = head->addr;

  /** Iterate and map each map. */
  map_t * curr = head;

  while (curr) {
    if (file == -1) {
      if (curr->file == -1) {
        curr->prot = prot;
        curr->flag = flag;
        curr->file = shmap_open(curr->size, NULL);
        curr->off  = 0;
      }
    } else {
      DEBUG_ASSERT(curr->file == -1);
      curr->prot = prot;
      curr->flag = flag;
      curr->file = file;
      curr->off  = off;
    }

    void * addr = _mmap(curr);
    DEBUG_ASSERT(addr == curr->addr);

    curr = curr->next;
  }

  _mapped = insert(_mapped, head);
  return ret;
}

