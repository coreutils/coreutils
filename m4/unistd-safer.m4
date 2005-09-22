#serial 7
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_UNISTD_SAFER],
[
  AC_LIBSOURCES([dup-safer.c, fd-safer.c, pipe-safer.c, unistd-safer.h, unistd--.h])
  AC_LIBOBJ([dup-safer])
  AC_LIBOBJ([fd-safer])
  AC_LIBOBJ([pipe-safer])
])
