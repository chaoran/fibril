EXEC = fib
CFLAGS = -Wall -pthread -g -O2
LDFLAGS = -pthread
LDLIBS = -lpthread

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)

-include $(DEPS)

%.d: %.c
	@set -e; rm -f $@; \
		$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

clean:
	rm -f $(EXEC) $(OBJS) $(DEPS) *.d.* core.* vgcore.*
