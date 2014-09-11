CC = gcc
CFLAGS = -Wall -pthread -g -O2 -DENABLE_SAFE
LDFLAGS = -pthread -lpthread

EXEC = fib
INCS = fibril.h stack.h
SRCS = fib.c init.c stack.c
OBJS = $(SRCS:.c=.o)

.PHONY: all run clean debug

$(EXEC): $(OBJS)

all: $(EXEC)

debug: CFLAGS += -DENABLE_DEBUG
debug: $(EXEC)

debug-l1: CFLAGS += -DENABLE_DEBUG -DDEBUG_LEVEL=1
debug-l1: $(EXEC)

debug-l2: CFLAGS += -DENABLE_DEBUG -DDEBUG_LEVEL=2
debug-l2: $(EXEC)

debug-l3: CFLAGS += -DENABLE_DEBUG -DDEBUG_LEVEL=3
debug-l3: $(EXEC)

run: $(EXEC)
		./$(EXEC)

clean:
		rm -f $(EXEC) $(OBJS) core.* vgcore.*
