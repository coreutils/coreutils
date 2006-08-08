dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_RANDREAD],
[
  AC_LIBSOURCES([randread.c, randread.h, rand-isaac.c, rand-isaac.h])
  AC_LIBOBJ([randread])
  AC_LIBOBJ([rand-isaac])
])
