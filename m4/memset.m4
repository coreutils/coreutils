# memset.m4 serial 2
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMSET],
[
  AC_REPLACE_FUNCS(memset)
  if test $ac_cv_func_memset = no; then
    gl_PREREQ_MEMSET
  fi
])

# Prerequisites of lib/memset.c.
AC_DEFUN([gl_PREREQ_MEMSET], [
  :
])
