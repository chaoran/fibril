AC_DEFUN([FIBRIL_IF_ENABLED_NOHELP],[
case "$enable_[]patsubst([$1], -, _)" in
  '' | no) :
      $3 ;;
  *)  $2 ;;
esac
])

AC_DEFUN([FIBRIL_IF_ENABLED],[
AC_MSG_CHECKING(whether to enable $1)
AC_ARG_ENABLE($1,AS_HELP_STRING(--enable-$1,[$2]))
FIBRIL_IF_ENABLED_NOHELP([$1],[$3],[$4])
AC_MSG_RESULT($enable_[]patsubst([$1], -, _))
])

