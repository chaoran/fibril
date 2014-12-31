#ifndef STACK_H
#define STACK_H

#include "fibrili.h"

void * stack_setup(struct _fibril_t * frptr);
void stack_reinstall(struct _fibril_t * frptr);
void stack_uninstall(struct _fibril_t * frptr);

#endif /* end of include guard: STACK_H */
