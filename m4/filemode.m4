# filemode.m4 serial 4
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FILEMODE],
[
  AC_LIBSOURCES([filemode.c, filemode.h, stat-macros.h])
  AC_LIBOBJ([filemode])

  dnl Prerequisites of lib/filemode.c.
  AC_REQUIRE([AC_HEADER_STAT])
])
