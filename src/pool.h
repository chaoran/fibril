#ifndef POOL_H
#define POOL_H

void pool_init();
void pool_put(void * stack);
void * pool_take(int force);

#endif /* end of include guard: POOL_H */
