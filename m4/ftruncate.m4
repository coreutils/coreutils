#serial 5

# See if we need to emulate a missing ftruncate function using fcntl or chsize.

AC_DEFUN([jm_FUNC_FTRUNCATE],
[
  AC_REPLACE_FUNCS(ftruncate)
  if test $ac_cv_func_ftruncate = no; then
    gl_PREREQ_FTRUNCATE
  fi
])

# Prerequisites of lib/ftruncate.c.
AC_DEFUN([gl_PREREQ_FTRUNCATE],
[
  AC_CHECK_HEADERS_ONCE(unistd.h)
  AC_CHECK_FUNCS(chsize)
])
