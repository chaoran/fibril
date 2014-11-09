#include "util.h"
#include "debug.h"
#include "deque.h"
#include "joint.h"
#include "sched.h"
#include "shmap.h"
#include "tlmap.h"
#include "fibril.h"

static deque_t ** _deqs;
static int __fibril_shared__ _done = 1;

extern int _nprocs;

void sched_init()
{
  int nprocs = _nprocs;
  SAFE_NZCALL(_deqs = malloc(sizeof(deque_t * [nprocs])));

  int i;
  for (i = 0; i < nprocs; ++i) {
    _deqs[i] = (void *) &fibrile_deq + TLS_OFFSETS[i];
  }

  joint_t * jtptr = joint_create();
  jtptr->stack.top = STACK_BOTTOM;
  jtptr->stack.btm = STACK_BOTTOM;
  jtptr->stack.off = STACK_OFFSETS[0];

  fibrile_deq.jtptr = jtptr;

  DEBUG_DUMP(2, "sched_init:", (jtptr, "%p"), (jtptr->stptr->off, "%ld"));
}

void sched_work(int me)
{
  int nprocs = _nprocs;
  joint_t * jtptr = joint_create();

  while (!trylock(&_done)) {
    int victim = rand() % nprocs;

    if (victim == me) continue;

    fibril_t * frptr = deque_steal(_deqs[victim], victim, jtptr);

    if (frptr == NULL) continue;

    if (frptr->jtp != jtptr) {
      free(jtptr);
      jtptr = frptr->jtp;
    }

    void * dest = jtptr->stptr->top;
    void * addr = dest + STACK_OFFSETS[victim];
    size_t size = STACK_BOTTOM - dest;

    memcpy(dest, addr, size);

    fibrile_deq.jtptr = jtptr;
    unlock(&jtptr->lock);
    sched_resume(frptr);
  }

  unlock(&_done);
  exit(0);
  /*if (me) exit(0);*/
  /*else sched_exit(-1);*/

  /*unreachable();*/
}

void sched_exit(int id)
{
  /*static __fibril_shared__ fibril_t fr;*/

  /*switch (id) {*/
    /*case  0: {*/
               /*unlock(&_done);*/
               /*break;*/
             /*}*/
    /*case -1: {*/
               /*joint_t * jtptr = fr.jtp;*/
               /*jtptr->stack.top = fr.rsp;*/
               /*joint_import(jtptr);*/
               /*sched_resume(&fr);*/
             /*}*/
    /*default: {*/
               /*fibril_make(&fr);*/
               /*fibrile_save(&fr, &&AFTER_EXIT);*/

               /*fibrile_deq.jtptr->stack.off = STACK_OFFSETS[id];*/
               /*fr.jtp = fibrile_deq.jtptr;*/

               /*unlock(&_done);*/
               /*sched_restart();*/
             /*}*/
  /*}*/

/*AFTER_EXIT:*/
  unlock(&_done);
  /*free(fr.jtp);*/
  free(_deqs);
  return;
}

