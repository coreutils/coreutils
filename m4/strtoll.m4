# strtoll.m4 serial 3
dnl Copyright (C) 2002, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOLL],
[
  dnl We don't need (and can't compile) the replacement strtoll
  dnl unless the type 'long long' exists.
  AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
  if test "$ac_cv_type_long_long" = yes; then
    AC_REPLACE_FUNCS(strtoll)
    if test $ac_cv_func_strtoll = no; then
      gl_PREREQ_STRTOLL
    fi
  fi
])

# Prerequisites of lib/strtoll.c.
AC_DEFUN([gl_PREREQ_STRTOLL], [
  :
])
