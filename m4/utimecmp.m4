#serial 1
dnl Copyright (C) 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_UTIMECMP],
[
  AC_LIBSOURCES([utimecmp.c, utimecmp.h, intprops.h])
  AC_LIBOBJ([utimecmp])

  dnl Prerequisites of lib/utimecmp.c.
  AC_REQUIRE([gl_TIMESPEC])
  AC_REQUIRE([gl_FUNC_UTIMES])
  :
])
