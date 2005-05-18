# localcharset.m4 serial 2
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_LOCALCHARSET],
[
  dnl Prerequisites of lib/localcharset.c.
  AC_CHECK_HEADERS_ONCE(stddef.h stdlib.h string.h)
  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_CHECK_FUNCS_ONCE(setlocale)

  dnl Prerequisites of the lib/Makefile.am snippet.
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([gl_GLIBC21])
])
