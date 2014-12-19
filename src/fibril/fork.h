#ifndef FIBRILI_FORK_H
#define FIBRILI_FORK_H

/** Helper macros. */
#define _fibrili_nth(...) _fibrili_nth_(__VA_ARGS__, ## __VA_ARGS__, \
    16, 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, \
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 0)
#define _fibrili_nth_(_1, _1_, _2, _22, _3, _33, _4, _44, _5, _55, \
    _6, _66, _7, _77, _8, _88, _9, _99, _10, _1010, _11, _1111, _12, _1212, \
    _13, _1313, _14, _1414, _15, _1515, _16, _1616, N, ...) N
#define _fibrili_concat(left, right) left##right

/**
 * Define fibrili_fork.
 * fibrili_fork has two versions: one that has a return value, and the other
 * one has no return value.
 */
#define fibrili_fork(...) _fibrili_fork_(_fibrili_nth(__VA_ARGS__), __VA_ARGS__)
#define _fibrili_fork_(n, ...) _fibrili_concat(_fibrili_fork_, n)(__VA_ARGS__)

#define _fibrili_fork_3(frptr, fn, args) do { \
  void _fibrili_##fn##_fork(_fibrili_defs args) { \
    fibril_t * f = (frptr); \
    if (!fibrili_setjmp(&f->state)) { \
      fibrili_push(f); \
      fn(_fibrili_args args); \
      if (!fibrili_pop()) { \
        fibrili_resume(f); \
      } \
    } \
  } \
  _fibrili_##fn##_fork args; \
} while (0)

#define _fibrili_fork_4(frptr, ret, fn, args) do { \
  __typeof__(ret) _fibrili_##fn##_fork(_fibrili_defs args) { \
    fibril_t * f = (frptr); \
    if (!fibrili_setjmp(&f->state)) {\
      fibrili_push(f); \
      ret = fn(_fibrili_args args); \
      if (!fibrili_pop()) { \
        void * p = &(ret); \
        * (volatile __typeof__(&ret)) p = ret; \
        fibrili_resume(f); \
      } \
    } \
    return ret; \
  } \
  ret = _fibrili_##fn##_fork args; \
} while (0)

#define _fibrili_defs(...) \
  _fibrili_defs_(_fibrili_nth(__VA_ARGS__), ## __VA_ARGS__)

#define _fibrili_defs_(n, ...) _fibrili_concat(_fibrili_defs_, n)(__VA_ARGS__)
#define _fibrili_defs_16(a, ...) __typeof__(a) a16,_fibrili_defs_15(__VA_ARGS__)
#define _fibrili_defs_15(a, ...) __typeof__(a) a15,_fibrili_defs_14(__VA_ARGS__)
#define _fibrili_defs_14(a, ...) __typeof__(a) a14,_fibrili_defs_13(__VA_ARGS__)
#define _fibrili_defs_13(a, ...) __typeof__(a) a13,_fibrili_defs_12(__VA_ARGS__)
#define _fibrili_defs_12(a, ...) __typeof__(a) a12,_fibrili_defs_11(__VA_ARGS__)
#define _fibrili_defs_11(a, ...) __typeof__(a) a11,_fibrili_defs_10(__VA_ARGS__)
#define _fibrili_defs_10(a, ...) __typeof__(a) a10,_fibrili_defs_9(__VA_ARGS__)
#define _fibrili_defs_9(a, ...) __typeof__(a) a9, _fibrili_defs_8(__VA_ARGS__)
#define _fibrili_defs_8(a, ...) __typeof__(a) a8, _fibrili_defs_7(__VA_ARGS__)
#define _fibrili_defs_7(a, ...) __typeof__(a) a7, _fibrili_defs_6(__VA_ARGS__)
#define _fibrili_defs_6(a, ...) __typeof__(a) a6, _fibrili_defs_5(__VA_ARGS__)
#define _fibrili_defs_5(a, ...) __typeof__(a) a5, _fibrili_defs_4(__VA_ARGS__)
#define _fibrili_defs_4(a, ...) __typeof__(a) a4, _fibrili_defs_3(__VA_ARGS__)
#define _fibrili_defs_3(a, ...) __typeof__(a) a3, _fibrili_defs_2(__VA_ARGS__)
#define _fibrili_defs_2(a, ...) __typeof__(a) a2, _fibrili_defs_1(__VA_ARGS__)
#define _fibrili_defs_1(a)      __typeof__(a) a1
#define _fibrili_defs_0()

#define _fibrili_args(...) \
  _fibrili_args_(_fibrili_nth(__VA_ARGS__), ## __VA_ARGS__)

#define _fibrili_args_(n, ...) _fibrili_concat(_fibrili_args_, n)(__VA_ARGS__)
#define _fibrili_args_16(a, ...) a16, _fibrili_args_15(__VA_ARGS__)
#define _fibrili_args_15(a, ...) a15, _fibrili_args_14(__VA_ARGS__)
#define _fibrili_args_14(a, ...) a14, _fibrili_args_13(__VA_ARGS__)
#define _fibrili_args_13(a, ...) a13, _fibrili_args_12(__VA_ARGS__)
#define _fibrili_args_12(a, ...) a12, _fibrili_args_11(__VA_ARGS__)
#define _fibrili_args_11(a, ...) a11, _fibrili_args_10(__VA_ARGS__)
#define _fibrili_args_10(a, ...) a10, _fibrili_args_9(__VA_ARGS__)
#define _fibrili_args_9(a, ...) a9, _fibrili_args_8(__VA_ARGS__)
#define _fibrili_args_8(a, ...) a8, _fibrili_args_7(__VA_ARGS__)
#define _fibrili_args_7(a, ...) a7, _fibrili_args_6(__VA_ARGS__)
#define _fibrili_args_6(a, ...) a6, _fibrili_args_5(__VA_ARGS__)
#define _fibrili_args_5(a, ...) a5, _fibrili_args_4(__VA_ARGS__)
#define _fibrili_args_4(a, ...) a4, _fibrili_args_3(__VA_ARGS__)
#define _fibrili_args_3(a, ...) a3, _fibrili_args_2(__VA_ARGS__)
#define _fibrili_args_2(a, ...) a2, _fibrili_args_1(__VA_ARGS__)
#define _fibrili_args_1(a)      a1
#define _fibrili_args_0()

#endif /* end of include guard: FIBRILI_FORK_H */
