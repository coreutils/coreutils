# serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STRIPSLASH],
[
  AC_LIBSOURCES([stripslash.c, dirname.h])
  AC_LIBOBJ([stripslash])

  dnl prerequisites
  AC_REQUIRE([gl_AC_DOS])
])
