# stpcpy.m4 serial 1
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STPCPY],
[
  dnl Persuade glibc <string.h> to declare stpcpy().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REPLACE_FUNCS(stpcpy)
  if test $ac_cv_func_stpcpy = no; then
    gl_PREREQ_STPCPY
  fi
])

# Prerequisites of lib/stpcpy.c.
AC_DEFUN([gl_PREREQ_STPCPY], [
  :
])

