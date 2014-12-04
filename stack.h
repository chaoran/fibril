#ifndef STACK_H
#define STACK_H

#include "fibril.h"

void stack_init();
void * stack_setup(fibril_t * frptr, void * stack);
void stack_reinstall(fibril_t * frptr);
void stack_uninstall(fibril_t * frptr);

#endif /* end of include guard: STACK_H */
