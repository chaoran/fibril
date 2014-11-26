#ifndef SCHED_H
#define SCHED_H

#include "fibril.h"

__attribute__((noreturn)) extern
void sched_resched(void);

__attribute__((noreturn)) extern
void sched_resume(fibril_t * frptr);

#endif /* end of include guard: SCHED_H */
