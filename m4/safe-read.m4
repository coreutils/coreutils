# safe-read.m4 serial 4
dnl Copyright (C) 2002-2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAFE_READ],
[
  AC_LIBSOURCES([safe-read.c, safe-read.h])
  AC_LIBOBJ([safe-read])

  gl_PREREQ_SAFE_READ
])

# Prerequisites of lib/safe-read.c.
AC_DEFUN([gl_PREREQ_SAFE_READ],
[
  AC_REQUIRE([gt_TYPE_SSIZE_T])
])
