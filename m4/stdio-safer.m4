# stdio-safer.m4 serial 2
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STDIO_SAFER],
[
  dnl Prerequisites of lib/fopen-safer.c.
  AC_CHECK_HEADERS_ONCE(unistd.h)
])
