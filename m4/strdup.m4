# strdup.m4 serial 5
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRDUP],
[
  AC_REPLACE_FUNCS(strdup)
  AC_CHECK_DECLS_ONCE(strdup)
  gl_PREREQ_STRDUP
])

# Prerequisites of lib/strdup.c.
AC_DEFUN([gl_PREREQ_STRDUP], [:])
