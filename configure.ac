#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fibril], [0.0.2], [chaoran@rice.edu])
AM_INIT_AUTOMAKE([-Wall -Wno-extra-portability -Werror foreign])
LT_PREREQ([2.2])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/fibril.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for command-line.
FIBRIL_IF_ENABLED([debug], [Build fibril in debugging mode],
  [
   case "${enable_debug}" in 1|2|3) ;; *) enable_debug=0 ;; esac
   AC_DEFINE_UNQUOTED([FIBRIL_DEBUG], [${enable_debug}], [Fibril debug enabled])
  ])

# Check for pthreads
ACX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS"
             CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
             CC="$PTHREAD_CC"])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h unistd.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([munmap])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 benchmark/Makefile])
AC_OUTPUT
