# filemode.m4 serial 6
dnl Copyright (C) 2002, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FILEMODE],
[
  AC_LIBSOURCES([filemode.c, filemode.h])
  AC_LIBOBJ([filemode])
  AC_CHECK_DECLS_ONCE([strmode])
])
