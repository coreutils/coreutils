# strtoul.m4 serial 2
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRTOUL],
[
  AC_REPLACE_FUNCS(strtoul)
  if test $ac_cv_func_strtoul = no; then
    gl_PREREQ_STRTOUL
  fi
])

# Prerequisites of lib/strtoul.c.
AC_DEFUN([gl_PREREQ_STRTOUL], [
  gl_PREREQ_STRTOL
])
