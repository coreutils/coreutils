# makepath.m4 serial 4
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MAKEPATH],
[
  dnl Prerequisites of lib/makepath.c.
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_CHECK_HEADERS_ONCE(unistd.h)
  AC_REQUIRE([AC_HEADER_STAT])
  AC_REQUIRE([gl_AFS])
])
