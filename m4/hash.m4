# hash.m4 serial 5
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_HASH],
[
  AC_LIBSOURCES([hash.c, hash.h])
  AC_LIBOBJ([hash])

  dnl Prerequisites of lib/hash.c.
  AC_REQUIRE([AM_STDBOOL_H])
])
