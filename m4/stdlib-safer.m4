# stdlib-safer.m4 serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STDLIB_SAFER],
[
  AC_LIBSOURCES([mkstemp-safer.c, stdlib-safer.h, stdlib--.h])
  AC_LIBOBJ([mkstemp-safer])
])
