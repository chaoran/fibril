AM_CPPFLAGS = -I$(includedir)
AM_LDFLAGS = -L$(libdir) -l$(PACKAGE)

check_PROGRAMS = \
                 fft \
                 fib \
                 heat \
                 integrate \
								 lu \
                 matmul \
                 nqueens \
                 quicksort

fft_LDADD = -lm
heat_LDADD = -lm
lu_LDADD = -lm

TESTS = $(check_PROGRAMS)
