#serial 10

dnl From Jim Meyering.
dnl Determine whether lstat has the bug that it succeeds when given the
dnl zero-length file name argument.  The lstat from SunOS 4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_LSTAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN([jm_FUNC_LSTAT],
[
  AC_FUNC_LSTAT
  dnl Note: AC_FUNC_LSTAT does AC_LIBOBJ(lstat).
  if test $ac_cv_func_lstat_empty_string_bug = yes; then
    gl_PREREQ_LSTAT
  fi
])

# Prerequisites of lib/lstat.c.
AC_DEFUN([gl_PREREQ_LSTAT],
[
  AC_REQUIRE([AC_HEADER_STAT])
  :
])
