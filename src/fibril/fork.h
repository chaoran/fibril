#ifndef FIBRIL_FORK_H
#define FIBRIL_FORK_H

#define _fibril_defs(...) \
  _fibril_defs_(_fibril_nth(__VA_ARGS__), ## __VA_ARGS__)
#define _fibril_defs_(n, ...) \
  _fibril_concat(_fibril_defs_, n)(__VA_ARGS__)
#define _fibril_defs_16(a,...) __typeof__(a) a16,_fibril_defs_15(__VA_ARGS__)
#define _fibril_defs_15(a,...) __typeof__(a) a15,_fibril_defs_14(__VA_ARGS__)
#define _fibril_defs_14(a,...) __typeof__(a) a14,_fibril_defs_13(__VA_ARGS__)
#define _fibril_defs_13(a,...) __typeof__(a) a13,_fibril_defs_12(__VA_ARGS__)
#define _fibril_defs_12(a,...) __typeof__(a) a12,_fibril_defs_11(__VA_ARGS__)
#define _fibril_defs_11(a,...) __typeof__(a) a11,_fibril_defs_10(__VA_ARGS__)
#define _fibril_defs_10(a,...) __typeof__(a) a10,_fibril_defs_9 (__VA_ARGS__)
#define _fibril_defs_9(a, ...) __typeof__(a) a9, _fibril_defs_8 (__VA_ARGS__)
#define _fibril_defs_8(a, ...) __typeof__(a) a8, _fibril_defs_7 (__VA_ARGS__)
#define _fibril_defs_7(a, ...) __typeof__(a) a7, _fibril_defs_6 (__VA_ARGS__)
#define _fibril_defs_6(a, ...) __typeof__(a) a6, _fibril_defs_5 (__VA_ARGS__)
#define _fibril_defs_5(a, ...) __typeof__(a) a5, _fibril_defs_4 (__VA_ARGS__)
#define _fibril_defs_4(a, ...) __typeof__(a) a4, _fibril_defs_3 (__VA_ARGS__)
#define _fibril_defs_3(a, ...) __typeof__(a) a3, _fibril_defs_2 (__VA_ARGS__)
#define _fibril_defs_2(a, ...) __typeof__(a) a2, _fibril_defs_1 (__VA_ARGS__)
#define _fibril_defs_1(a)      __typeof__(a) a1,
#define _fibril_defs_0()

#define _fibril_args(...) \
  _fibril_args_(_fibril_nth(__VA_ARGS__), ## __VA_ARGS__)
#define _fibril_args_(n, ...) \
  _fibril_concat(_fibril_args_, n)(__VA_ARGS__)
#define _fibril_args_16(a,...) a16,_fibril_args_15(__VA_ARGS__)
#define _fibril_args_15(a,...) a15,_fibril_args_14(__VA_ARGS__)
#define _fibril_args_14(a,...) a14,_fibril_args_13(__VA_ARGS__)
#define _fibril_args_13(a,...) a13,_fibril_args_12(__VA_ARGS__)
#define _fibril_args_12(a,...) a12,_fibril_args_11(__VA_ARGS__)
#define _fibril_args_11(a,...) a11,_fibril_args_10(__VA_ARGS__)
#define _fibril_args_10(a,...) a10,_fibril_args_9 (__VA_ARGS__)
#define _fibril_args_9(a, ...) a9, _fibril_args_8 (__VA_ARGS__)
#define _fibril_args_8(a, ...) a8, _fibril_args_7 (__VA_ARGS__)
#define _fibril_args_7(a, ...) a7, _fibril_args_6 (__VA_ARGS__)
#define _fibril_args_6(a, ...) a6, _fibril_args_5 (__VA_ARGS__)
#define _fibril_args_5(a, ...) a5, _fibril_args_4 (__VA_ARGS__)
#define _fibril_args_4(a, ...) a4, _fibril_args_3 (__VA_ARGS__)
#define _fibril_args_3(a, ...) a3, _fibril_args_2 (__VA_ARGS__)
#define _fibril_args_2(a, ...) a2, _fibril_args_1 (__VA_ARGS__)
#define _fibril_args_1(a)      a1
#define _fibril_args_0()

#define _fibril_expand(...) \
  _fibril_expand_(_fibril_nth(__VA_ARGS__), ## __VA_ARGS__)
#define _fibril_expand_(n, ...) \
  _fibril_concat(_fibril_expand_, n)(__VA_ARGS__)
#define _fibril_expand_16(...) __VA_ARGS__,
#define _fibril_expand_15(...) __VA_ARGS__,
#define _fibril_expand_14(...) __VA_ARGS__,
#define _fibril_expand_13(...) __VA_ARGS__,
#define _fibril_expand_12(...) __VA_ARGS__,
#define _fibril_expand_11(...) __VA_ARGS__,
#define _fibril_expand_10(...) __VA_ARGS__,
#define _fibril_expand_9( ...) __VA_ARGS__,
#define _fibril_expand_8( ...) __VA_ARGS__,
#define _fibril_expand_7( ...) __VA_ARGS__,
#define _fibril_expand_6( ...) __VA_ARGS__,
#define _fibril_expand_5( ...) __VA_ARGS__,
#define _fibril_expand_4( ...) __VA_ARGS__,
#define _fibril_expand_3( ...) __VA_ARGS__,
#define _fibril_expand_2( ...) __VA_ARGS__,
#define _fibril_expand_1( ...) __VA_ARGS__,
#define _fibril_expand_0()

#endif /* end of include guard: FIBRIL_FORK_H */
