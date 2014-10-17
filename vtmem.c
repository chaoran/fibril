#include <string.h>
#include <unistd.h>

#include "tls.h"
#include "safe.h"
#include "page.h"
#include "debug.h"
#include "shmap.h"
#include "stack.h"

char __data_start, _end;
FIBRILi(tls_t) FIBRILi(tls);

#define _tls (FIBRILi(tls))
static shmap_t ** _tls_maps;
extern shmap_t ** _stack_maps;

static void get_data_range(void ** addr, size_t * size)
{
  *addr = PAGE_ALIGN_DOWN(&__data_start);
  *size = PAGE_ALIGN_UP(&_end) - *addr;

  SAFE_ASSERT(PAGE_ALIGNED(*addr));
  SAFE_ASSERT(PAGE_DIVISIBLE(*size));
}

static void get_tls_range(void ** addr, size_t * size)
{
  *addr = &_tls;
  *size = sizeof(_tls);

  SAFE_ASSERT(PAGE_ALIGNED(*addr));
  SAFE_ASSERT(PAGE_DIVISIBLE(*size));
}

static shmap_t **
create_tlmap(int nprocs, shmap_t * map, const char * name)
{
  char path[FILENAME_LIMIT];
  size_t size = map->size;

  shmap_mktl(map, nprocs);

  shmap_t ** maps = malloc(sizeof(shmap_t * [nprocs]));
  maps[0] = shmap_mmap(NULL, size, map->file.tl[0]);

  int i;
  for (i = 1; i < nprocs; ++i) {
    sprintf(path, "%s_%d", name, i);
    maps[i] = shmap_mmap(NULL, size, shmap_open(size, path));
  }

  return maps;
}

static void tls_init(int id, FIBRILi(tls_t) * tls)
{
  tls->x.tid = id;
  tls->x.pid = getpid();
}

void vtmem_init(int nprocs)
{
  tls_init(0, &FIBRILi(tls));

  void * data_addr;
  size_t data_size;

  get_data_range(&data_addr, &data_size);
  DEBUG_PRINT_INFO("data: addr=%p size=%ld\n", data_addr, data_size);

  shmap_copy(data_addr, data_size, "data");

  void * tls_addr;
  size_t tls_size;

  get_tls_range(&tls_addr, &tls_size);
  DEBUG_PRINT_INFO("tls: addr=%p size=%ld\n", tls_addr, tls_size);

  shmap_t * map = shmap_copy(tls_addr, tls_size, "tls");
  _tls_maps = create_tlmap(nprocs, map, "tls");

  void * stack_addr;
  size_t stack_size;

  stack_range(&stack_addr, &stack_size);
  map = stack_copy(stack_addr, stack_size);
  _stack_maps = create_tlmap(nprocs, map, "stack");
}

void vtmem_init_thread(int id)
{
  if (id == 0) return;

  void * tls_addr;
  size_t tls_size;
  get_tls_range(&tls_addr, &tls_size);

  tls_init(id, _tls_maps[id]->addr);
  shmap_mmap(tls_addr, tls_size, _tls_maps[id]->file.sh);

  void * stack_addr;
  size_t stack_size;
  stack_range(&stack_addr, &stack_size);

  shmap_mmap(stack_addr, stack_size, _stack_maps[id]->file.sh);
}

