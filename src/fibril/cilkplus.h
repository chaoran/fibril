#ifndef CILKPLUS_H
#define CILKPLUS_H

#include <cilk/cilk.h>

#define fibril
#define fibril_t __attribute__((unused)) int
#define fibril_init(fp)
#define fibril_join(fp) cilk_sync

#define fibril_fork_nrt(fp, fn, ag)     (cilk_spawn fn ag)
#define fibril_fork_wrt(fp, rt, fn, ag) (rt = cilk_spawn fn ag)

#define fibril_rt_init(n)
#define fibril_rt_exit()
#define fibril_rt_nprocs(n) \
  (n > 0 ? __cilkrts_set_param("nworkers", #n) : __cilkrts_get_nworkers())

#endif /* end of include guard: CILKPLUS_H */
