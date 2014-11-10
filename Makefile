CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -Wl,-T -Wl,fibril.ld
LDLIBS = -lrt -lstdc++ -lm -ldl

EXEC = fib

INCS = \
			 config.h \
			 debug.h \
			 deque.h \
			 fibril.h \
			 fibrile.h \
			 fibrili.h \
			 joint.h \
			 page.h \
			 safe.h \
			 sched.h \
			 shmap.h \
			 stack.h \
			 tls.h \
			 util.h \

SRCS = \
			 exit.c \
			 deque.c \
			 fib.c \
			 init.c \
			 join.c \
			 sched.c \
			 shmap.c \
			 stack.c \
			 tls.c \

HOARD_DIR = Hoard/src
HOARD_OBJS = $(addprefix $(HOARD_DIR)/, libhoard.o unixtls.o gnuwrapper.o)
OBJS = $(SRCS:.c=.o) $(HOARD_OBJS)

.PHONY: all run clean debug

$(EXEC): $(OBJS) $(INCS)

all: $(EXEC)

debug: CFLAGS += -DENABLE_DEBUG -DDEBUG_WAIT=1
debug: clean $(EXEC)

debug-l1: CFLAGS += -DENABLE_DEBUG -DDEBUG_WAIT=1 -DDEBUG_LEVEL=1
debug-l1: clean $(EXEC)

debug-l2: CFLAGS += -DENABLE_DEBUG -DDEBUG_WAIT=1 -DDEBUG_LEVEL=2
debug-l2: clean $(EXEC)

debug-l3: CFLAGS += -DENABLE_DEBUG -DDEBUG_WAIT=1 -DDEBUG_LEVEL=3
debug-l3: clean $(EXEC)

$(HOARD_OBJS):
	$(MAKE) -C $(HOARD_DIR) linux-gcc-x86-64-static

run: $(EXEC)
		./$(EXEC)

clean:
	$(MAKE) -C $(HOARD_DIR) clean
	rm -f $(EXEC) *.o core.* vgcore.*
