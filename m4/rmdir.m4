# rmdir.m4 serial 2
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_RMDIR],
[
  AC_REPLACE_FUNCS(rmdir)
  if test $ac_cv_func_rmdir = no; then
    gl_PREREQ_RMDIR
  fi
])

# Prerequisites of lib/rmdir.c.
AC_DEFUN([gl_PREREQ_RMDIR], [
  AC_REQUIRE([AC_HEADER_STAT])
  :
])
