# safe-read.m4 serial 2
dnl Copyright (C) 2002-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAFE_READ],
[
  gl_PREREQ_SAFE_READ
])

# Prerequisites of lib/safe-read.c.
AC_DEFUN([gl_PREREQ_SAFE_READ],
[
  AC_REQUIRE([gt_TYPE_SSIZE_T])
  AC_CHECK_HEADERS_ONCE(unistd.h)
])
