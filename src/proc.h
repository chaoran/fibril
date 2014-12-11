#ifndef PROC_H
#define PROC_H

#include "fibril.h"

__attribute__((noreturn)) extern
void proc_restart(fibril_t * frptr);

__attribute__((noreturn)) extern
void proc_resume(const fibril_t * frptr);

void proc_start(int id, int nprocs);
void proc_stop (void);

#endif /* end of include guard: PROC_H */
