#serial 3
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Paul Eggert.

AC_DEFUN([gl_XNANOSLEEP],
[
  AC_LIBSOURCES([xnanosleep.c, xnanosleep.h, intprops.h])
  AC_LIBOBJ([xnanosleep])
])
