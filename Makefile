EXEC = fib
CFLAGS = -Wall -pthread -g -O2
LDFLAGS = -pthread
LDLIBS = -lpthread

INCS = \
			 atomic.h \
			 fibril.h \
			 fibrili.h \
			 sched.h \

SRCS = \
			 fib.c \
			 fibril.c \

OBJS = $(SRCS:.c=.o)

$(EXEC): $(OBJS) $(INCS)

all: $(EXEC)

clean:
	rm -f $(EXEC) $(OBJS) core.* vgcore.*
