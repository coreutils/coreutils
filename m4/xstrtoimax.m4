#serial 5
dnl Cloned from xstrtoumax.m4.  Keep these files in sync.

dnl Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_XSTRTOIMAX],
[
  dnl Prerequisites of lib/xstrtoimax.c.
  AC_REQUIRE([gl_AC_TYPE_INTMAX_T])
  AC_REQUIRE([gl_PREREQ_XSTRTOL])
])
