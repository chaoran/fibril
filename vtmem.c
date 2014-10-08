#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "page.h"
#include "safe.h"
#include "debug.h"
#include "globals.h"

#define FILENAME_LIMIT 256

typedef struct _map_t {
  int fd;
  struct {
    void * start;
    void * end;
  } addr;
  struct {
    char r;
    char w;
    char x;
    char p;
  } perm;
  size_t offset;
  char pathname[FILENAME_LIMIT];
  struct _map_t * next;
} map_t;

static map_t * load_maps()
{
  FILE * maps;
  SAFE_RETURN(maps, fopen("/proc/self/maps", "r"));

  map_t * list = NULL;
  map_t * map = malloc(sizeof(map_t));
  map->fd = 0;
  map->pathname[0] = '\0';

  while (EOF != fscanf(maps, "%p-%p %c%c%c%c %lx %*d:%*d %*d %[[/]%s\n",
        &map->addr.start, &map->addr.end,
        &map->perm.r, &map->perm.w, &map->perm.x, &map->perm.p,
        &map->offset, map->pathname, map->pathname + 1)) {

    DEBUG_PRINT_INFO("map: %18p-%18p %c%c%c%c 0x%08lx %s\n",
        map->addr.start, map->addr.end,
        map->perm.r, map->perm.w, map->perm.x, map->perm.p,
        map->offset, map->pathname);

    map->next = list;
    list = map;

    map = malloc(sizeof(map_t));
    map->fd = 0;
    map->pathname[0] = '\0';
  }

  free(map);

  SAFE_FNCALL(fclose(maps));
  return list;
}

static int share_maps_by_name(map_t * map, const char * name)
{
  int count = 0;

  while (map) {
    if ('p' == map->perm.p && 'w' == map->perm.w) {
      if ((NULL == name && 0 == map->pathname[0])
          || (NULL != name && NULL != strstr(map->pathname, name))) {
        map->fd = page_expose(map->addr.start, map->addr.end - map->addr.start);
        map->perm.p = 's';
        count++;
      }
    }

    map = map->next;
  }

  return count;
}

static int share_maps_by_range(map_t * map, const void * addr, size_t size)
{
  int count = 0;
  const void * end = addr + size;

  while (map) {
    if ('w' == map->perm.w && 'p' == map->perm.p) {
      if ((map->addr.start <= addr && map->addr.end > addr)
          || (map->addr.start < end && map->addr.end >= end)) {
        map->fd = page_expose(map->addr.start, map->addr.end - map->addr.start);
        map->perm.p = 's';
        count++;
      }
    }

    map = map->next;
  }

  return count;
}

void vtmem_init()
{
  /** Load virtual address maps */
  map_t * maps = load_maps();

  SAFE_ASSERT(0 < share_maps_by_range(maps, GLOBALS_ALIGNED_RANGE));
  SAFE_ASSERT(0 < share_maps_by_name (maps, "libhoard"));
}

void vtmem_dump()
{
  load_maps();
}

