# strstr.m4 serial 2
dnl Copyright (C) 2002-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRSTR],
[
  AC_REPLACE_FUNCS(strstr)
  if test $ac_cv_func_strstr = no; then
    gl_PREREQ_STRSTR
  fi
])

# Prerequisites of lib/strstr.c.
AC_DEFUN([gl_PREREQ_STRSTR], [:])
