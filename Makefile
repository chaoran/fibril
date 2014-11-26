EXEC = fib
CFLAGS = -Wall -pthread -g -O2
LDFLAGS = -pthread
LDLIBS = -lpthread
OUTPUT_OPTION = -MMD -MP -o $@

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

-include $(DEPS)

$(EXEC): $(OBJS) $(INCS)

all: $(EXEC)

clean:
	rm -f $(EXEC) $(OBJS) $(DEPS) core.* vgcore.*
