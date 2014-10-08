#ifndef FIBRIL_H
#define FIBRIL_H

#include "fibrili.h"

#define FIBRIL_FORK(fcall, ...) do { \
  FIBRILi_SAVE(__VA_ARGS__) \
  fcall; \
  FIBRILi_REST(__VA_ARGS__) \
} while (0)

int fibril_init(int nprocs);
int fibril_exit();

#define FIBRIL_SUCCESS 0

#endif /* end of include guard: FIBRIL_H */
