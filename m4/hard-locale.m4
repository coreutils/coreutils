# hard-locale.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl No prerequisites of lib/hard-locale.c.
AC_DEFUN([gl_HARD_LOCALE],
[
  AC_LIBSOURCES([hard-locale.c, hard-locale.h])
  AC_LIBOBJ([hard-locale])
])
