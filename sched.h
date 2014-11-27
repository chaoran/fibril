#ifndef SCHED_H
#define SCHED_H

#include "fibril.h"

__attribute__((noreturn)) extern
void sched_restart(fibril_t * frptr);

__attribute__((noreturn)) extern
void sched_resume(const fibril_t * frptr);

void sched_start(int id, int nprocs);
void sched_stop (void);

#endif /* end of include guard: SCHED_H */
