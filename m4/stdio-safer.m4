#serial 7
dnl Copyright (C) 2002, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FOPEN_SAFER],
[
  AC_LIBSOURCES([fopen-safer.c, stdio-safer.h, stdio--.h])
  AC_LIBOBJ([fopen-safer])
])

AC_DEFUN([gl_TMPFILE_SAFER],
[
  AC_LIBSOURCES([tmpfile-safer.c, stdio-safer.h, stdio--.h])
  AC_LIBOBJ([tmpfile-safer])
])
