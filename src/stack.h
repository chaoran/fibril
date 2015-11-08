#ifndef STACK_H
#define STACK_H

#include <signal.h>
#include "fibrili.h"

void stack_init(int id);
void * stack_setup(struct _fibril_t * frptr);
void stack_reinstall(struct _fibril_t * frptr);
int stack_uninstall(struct _fibril_t * frptr);
void handle_segfault(int s, siginfo_t * si, void * unused);

#endif /* end of include guard: STACK_H */
