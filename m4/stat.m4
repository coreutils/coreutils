#serial 9

dnl From Jim Meyering.
dnl Determine whether stat has the bug that it succeeds when given the
dnl zero-length file name argument.  The stat from SunOS 4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_STAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN([jm_FUNC_STAT],
[
  AC_FUNC_STAT
  dnl Note: AC_FUNC_STAT does AC_LIBOBJ(stat).
  if test $ac_cv_func_stat_empty_string_bug = yes; then
    gl_PREREQ_STAT
  fi
])

# Prerequisites of lib/stat.c.
AC_DEFUN([gl_PREREQ_STAT],
[
  :
])
