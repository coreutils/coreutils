# strpbrk.m4 serial 2
dnl Copyright (C) 2002-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRPBRK],
[
  AC_REPLACE_FUNCS(strpbrk)
  if test $ac_cv_func_strpbrk = no; then
    gl_PREREQ_STRPBRK
  fi
])

# Prerequisites of lib/strpbrk.c.
AC_DEFUN([gl_PREREQ_STRPBRK], [:])
