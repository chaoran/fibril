#ifndef TBB_H
#define TBB_H

#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>

#define fibril
#define fibril_t tbb::task_group
#define fibril_init(fp)
#define fibril_join(fp) (fp)->wait()

#define fibril_fork_nrt(fp, fn, ag) (fp)->run([=]{ fn ag; })
#define fibril_fork_wrt(fp, rtp, fn, ag) do { \
  __typeof__(rtp) pt = rtp; \
  (fp)->run([=]{ *pt = fn ag; }); \
} while (0)

extern "C" {
  extern int PARAM_NPROCS;
  extern int fibril_rt_nprocs();
}

#define fibril_rt_init(n) \
  do { \
    int max_nprocs = fibril_rt_nprocs(); \
    if (n > 0 && n <= max_nprocs) { \
      PARAM_NPROCS = n; \
    } else { \
      PARAM_NPROCS = max_nprocs; \
    } \
  } while(0); \
tbb::task_scheduler_init _fibril_rt_init(PARAM_NPROCS)

#define fibril_rt_exit()

#endif /* end of include guard: TBB_H */
