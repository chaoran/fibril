#include "tls.h"
#include "safe.h"
#include "page.h"
#include "debug.h"
#include "shmap.h"
#include "stack.h"

char __data_start, _end;
tls_t _tls;

void vtmem_init(int nprocs)
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

void vtmem_init_thread(int id)
{
  /** Only child threads need to do this. */
  if (id == 0) return;

  shmap_map(TLS.stack.addr, TLS.stack.size, TLS.stack.maps[id].file);
}

