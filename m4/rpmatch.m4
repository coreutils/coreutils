# rpmatch.m4 serial 5
dnl Copyright (C) 2002, 2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_RPMATCH],
[
  AC_REPLACE_FUNCS(rpmatch)
  if test $ac_cv_func_rpmatch = no; then
    gl_PREREQ_RPMATCH
  fi
])

# Prerequisites of lib/rpmatch.c.
AC_DEFUN([gl_PREREQ_RPMATCH], [:])
