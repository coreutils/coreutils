#serial 4

# See if we need to emulate a missing ftruncate function using fcntl or chsize.

AC_DEFUN([jm_FUNC_FTRUNCATE],
[
  AC_CHECK_FUNCS(ftruncate, , [ftruncate_missing=yes])

  if test "$ftruncate_missing" = yes; then
    AC_CHECK_HEADERS([unistd.h])
    AC_CHECK_FUNCS([chsize])
    AC_LIBOBJ(ftruncate)
  fi
])
