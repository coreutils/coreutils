# fcntl-safer.m4 serial 3

# Copyright (C) 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

AC_DEFUN([gl_FCNTL_SAFER],
[
  gl_PREREQ_OPEN_SAFER
])

# Prerequisites of lib/open-safer.c.
AC_DEFUN([gl_PREREQ_OPEN_SAFER], [
  AC_CHECK_HEADERS_ONCE(fcntl.h unistd.h)
])
