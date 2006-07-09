# strtod.m4 serial 5
dnl Copyright (C) 2002, 2003, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOD],
[
  AC_REQUIRE([AC_FUNC_STRTOD])
  if test $ac_cv_func_strtod = no; then
    AC_DEFINE(strtod, rpl_strtod,
      [Define to rpl_strtod if the replacement function should be used.])
    gl_PREREQ_STRTOD
  fi
])

# Prerequisites of lib/strtod.c.
# The need for pow() is already handled by AC_FUNC_STRTOD.
AC_DEFUN([gl_PREREQ_STRTOD], [:])
