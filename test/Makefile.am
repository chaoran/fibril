AM_CPPFLAGS = -I$(includedir)
AM_LDFLAGS = -L$(libdir) -l$(PACKAGE)

check_PROGRAMS = \
                 fft \
                 fib \
                 integrate \
                 matmul \
                 nqueens \
                 quicksort

fft_LDADD = -lm

TESTS = $(check_PROGRAMS)
