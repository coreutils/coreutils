#serial 7
dnl Cloned from xstrtoimax.m4.  Keep these files in sync.

dnl Copyright (C) 1999, 2000, 2001, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_XSTRTOUMAX],
[
  dnl Prerequisites of lib/xstrtoumax.c.
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([gl_PREREQ_XSTRTOL])
])
