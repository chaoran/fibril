#ifndef GLOBALS_H
#define GLOBALS_H

#include "page.h"

extern char __data_start, _end;

#define GLOBALS_ALIGNED_ADDR (PAGE_ALIGN_DOWN(&__data_start))
#define GLOBALS_ALIGNED_SIZE (PAGE_ALIGN_UP  (&_end) - GLOBALS_ALIGNED_ADDR)

#endif /* end of include guard: GLOBALS_H */
