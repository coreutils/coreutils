# cloexec.m4 serial 2
dnl Copyright (C) 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CLOEXEC],
[
  dnl Prerequisites of lib/cloexec.c.
  AC_CHECK_HEADERS_ONCE(fcntl.h unistd.h)
])
