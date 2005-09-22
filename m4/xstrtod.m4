#serial 4
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prerequisites of lib/xstrtod.c.
AC_DEFUN([gl_XSTRTOD],
[
  AC_LIBSOURCES([xstrtod.c, xstrtod.h])
  AC_LIBOBJ([xstrtod])
])
