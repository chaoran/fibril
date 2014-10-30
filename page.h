#ifndef PAGE_H
#define PAGE_H

#include <stddef.h>
#include "config.h"

#define PAGE_SIZE_MASK (PAGE_SIZE - 1)

#define PAGE_ALIGN_DOWN(x) (\
    (void *) ((size_t) (x) & ~PAGE_SIZE_MASK) \
)

#define PAGE_ALIGN_UP(x) (\
    (void *) ((size_t) ((x) + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK) \
)

#define PAGE_ALIGNED(x) (\
    0 == ((size_t) (x) & PAGE_SIZE_MASK) \
)

#define PAGE_DIVISIBLE(x) (\
    0 == ((x) % PAGE_SIZE) \
)

#endif /* end of include guard: PAGE_H */
