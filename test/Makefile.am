AM_CPPFLAGS = -I$(includedir)
AM_LDFLAGS = -L$(libdir)
LIBS += -l$(PACKAGE) -lm

check_PROGRAMS = \
                 fft \
                 fib \
                 integrate \
                 matmul \
                 quicksort

TESTS = $(check_PROGRAMS)
