#ifndef FIBRIL_SERIAL_H
#define FIBRIL_SERIAL_H

#define fibril
#define fibril_t __attribute__((unused)) int
#define fibril_init(fp)
#define fibril_join(fp)

#define fibril_fork_nrt(fp, fn, ag) (fn ag)
#define fibril_fork_wrt(fp, rt, fn, ag) (rt = fn ag)

#define fibril_rt_init(n)
#define fibril_rt_exit()
#define fibril_rt_nprocs(n) (1)

#endif /* end of include guard: FIBRIL_SERIAL_H */
