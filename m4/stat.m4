#serial 11

# Copyright (C) 1997, 1998, 1999, 2000, 2001, 2003, 2004 Free Software
# Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.
dnl Determine whether stat has the bug that it succeeds when given the
dnl zero-length file name argument.  The stat from SunOS 4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_STAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN([gl_FUNC_STAT],
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
