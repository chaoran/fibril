#ifndef SCHED_H
#define SCHED_H

#include "util.h"
#include "stack.h"
#include "fibril.h"
#include "fibrili.h"

void sched_init(int nprocs);
__attribute__ ((noreturn))
void sched_work(int id, int nprocs);
void sched_exit();

static inline __attribute__ ((noreturn))
void sched_restart()
{
  DEBUG_DUMP(2, "restart");
  int id = TID;
  STACK_EXECUTE(STACK_ADDRS[id], sched_work(id, _nprocs));
}

__attribute__((noreturn))
void sched_resume(struct _fibrile_joint_t * jtptr, const fibril_t * frptr);

#endif /* end of include guard: SCHED_H */
