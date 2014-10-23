#ifndef STACK_H
#define STACK_H

void * stack_top(int id);
void * stack_alloc();
void   stack_free(void * top);
void   stack_init(int nprocs);
void   stack_init_child(int id);

#endif /* end of include guard: STACK_H */
