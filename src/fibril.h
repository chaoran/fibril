#ifndef FIBRIL_H
#define FIBRIL_H

#define FIBRIL_SUCCESS 0
#define FIBRIL_FAILURE -1

/**
 * These are special arguments to fibril_rt_init().
 * FIBRIL_NPROCS tells the runtime to fetch the number of processors
 * from the environment variable FIBRIL_NPROCS (getenv(FIBRIL_NPROCS)).
 * FIBRIL_NPROCS_ONLN tells the runtime to use all available processors
 * in the system (sysconf(_SC_NPROCESSORS_ONLN)).
 */
#define FIBRIL_NPROCS 0
#define FIBRIL_NPROCS_ONLN -1

/** Serial version. */
#ifdef FIBRIL_SERIAL
#include <fibril/serial.h>

/** Fibril version. */
#else
#include <fibril/fibrile.h>
#endif

/** fibril_fork has two versions: one with return value and one without. */
#define fibril_fork(...) _fibril_fork_(_fibril_nth(__VA_ARGS__), __VA_ARGS__)
#define _fibril_fork_(n, ...) _fibril_concat(_fibril_fork_, n)(__VA_ARGS__)

/** If nargs is 3, use the no-return-value version. */
#define _fibril_fork_3(...) fibril_fork_nrt(__VA_ARGS__)

/** If nargs is 4, use the with-return-value version. */
#define _fibril_fork_4(...) fibril_fork_wrt(__VA_ARGS__)

/** Helper macros to count number of arguments. */
#define _fibril_nth(...) _fibril_nth_(__VA_ARGS__, ## __VA_ARGS__, \
    16, 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, \
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 0)
#define _fibril_nth_(_1, _1_, _2, _2_, _3, _3_, _4, _4_, _5, _5_, \
    _6, _6_, _7, _7_, _8, _8_, _9, _9_, _10, _10_, _11, _11_, _12, _12_, \
    _13, _13_, _14, _14_, _15, _15_, _16, _16_, N, ...) N
#define _fibril_concat(left, right) left##right

#endif /* end of include guard: FIBRIL_H */
