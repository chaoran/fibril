#ifndef FIBRIL_H
#define FIBRIL_H

#include "fibrile.h"

#define FIBRIL_FORK(retval, fcall, ...) do { \
  FIBRILe_SAVE(__VA_ARGS__) \
  retval = fcall; \
  FIBRILe_REST(__VA_ARGS__) \
} while (0)

int fibril_init(int nprocs);
int fibril_exit();

#define FIBRIL_SUCCESS 0

#endif /* end of include guard: FIBRIL_H */
