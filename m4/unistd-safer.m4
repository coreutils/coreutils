# unistd-safer.m4 serial 2
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_UNISTD_SAFER],
[
  gl_PREREQ_DUP_SAFER
])

# Prerequisites of lib/dup-safer.c.
AC_DEFUN([gl_PREREQ_DUP_SAFER], [
  AC_CHECK_HEADERS_ONCE(fcntl.h unistd.h)
])
