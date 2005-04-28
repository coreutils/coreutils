# filemode.m4 serial 5
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FILEMODE],
[
  AC_LIBSOURCES([filemode.c, filemode.h])
  AC_LIBOBJ([filemode])
])
