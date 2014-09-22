#ifndef PAGE_H
#define PAGE_H

#include <stddef.h>

#define PAGE_SIZE      4096ULL
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

int page_expose(void * addr, size_t size);
void * page_map(int fd, void * addr, size_t size);

#endif /* end of include guard: PAGE_H */
