#ifndef STACK_H
#define STACK_H

void stack_init(int nprocs);
void stack_init_thread(int id);
void * stack_top(int id);

#endif /* end of include guard: STACK_H */
