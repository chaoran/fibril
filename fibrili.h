#ifndef FIBRILI_H
#define FIBRILI_H

/** Macros to count the number of variadic macro arguments. */
#define FIBRILi_CONCAT(left, right) left##right
#define FIBRILi_NARG(...) FIBRILi_NARG_(__VA_ARGS__, ##__VA_ARGS__, \
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 0)
#define FIBRILi_NARG_(...) FIBRILi_ARG_N(__VA_ARGS__)
#define FIBRILi_ARG_N(_1, _11, _2, _22, _3, _33, _4, _44, _5, _55, \
    _6, _66, _7, _77, _8, _88, N, ...) N

/** FIBRILi_SAVE */
#define FIBRILi_SAVE(...) \
  FIBRILi_SAVE_(FIBRILi_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define FIBRILi_SAVE_(N, ...) \
  FIBRILi_CONCAT(FIBRILi_SAVE_, N)(__VA_ARGS__)
#define FIBRILi_SAVE_d(type, var) type volatile _fr_##var
#define FIBRILi_SAVE_v(type, var) var
#define FIBRILi_SAVE_s(var) FIBRILi_SAVE_d var = FIBRILi_SAVE_v var;
#define FIBRILi_SAVE_0(...)
#define FIBRILi_SAVE_1(var, ...) FIBRILi_SAVE_s(var)
#define FIBRILi_SAVE_2(var, ...) FIBRILi_SAVE_s(var) FIBRILi_SAVE_1(__VA_ARGS__)
#define FIBRILi_SAVE_3(var, ...) FIBRILi_SAVE_s(var) FIBRILi_SAVE_2(__VA_ARGS__)
#define FIBRILi_SAVE_4(var, ...) FIBRILi_SAVE_s(var) FIBRILi_SAVE_3(__VA_ARGS__)
#define FIBRILi_SAVE_5(var, ...) FIBRILi_SAVE_s(var) FIBRILi_SAVE_4(__VA_ARGS__)
#define FIBRILi_SAVE_6(var, ...) FIBRILi_SAVE_s(var) FIBRILi_SAVE_5(__VA_ARGS__)
#define FIBRILi_SAVE_7(var, ...) FIBRILi_SAVE_s(var) FIBRILi_SAVE_6(__VA_ARGS__)

/** FIBRILi_REST */
#define FIBRILi_REST(...) \
  FIBRILi_REST_(FIBRILi_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define FIBRILi_REST_(N, ...) \
  FIBRILi_CONCAT(FIBRILi_REST_, N)(__VA_ARGS__)
#define FIBRILi_REST_a(type, var) var = _fr_##var
#define FIBRILi_REST_s(var) FIBRILi_REST_a var;
#define FIBRILi_REST_0(...)
#define FIBRILi_REST_1(var, ...) FIBRILi_REST_s(var)
#define FIBRILi_REST_2(var, ...) FIBRILi_REST_s(var) FIBRILi_REST_1(__VA_ARGS__)
#define FIBRILi_REST_3(var, ...) FIBRILi_REST_s(var) FIBRILi_REST_2(__VA_ARGS__)
#define FIBRILi_REST_4(var, ...) FIBRILi_REST_s(var) FIBRILi_REST_3(__VA_ARGS__)
#define FIBRILi_REST_5(var, ...) FIBRILi_REST_s(var) FIBRILi_REST_4(__VA_ARGS__)
#define FIBRILi_REST_6(var, ...) FIBRILi_REST_s(var) FIBRILi_REST_5(__VA_ARGS__)
#define FIBRILi_REST_7(var, ...) FIBRILi_REST_s(var) FIBRILi_REST_6(__VA_ARGS__)

#endif /* end of include guard: FIBRILI_H */
