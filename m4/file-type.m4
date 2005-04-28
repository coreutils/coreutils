# file-type.m4 serial 5
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FILE_TYPE],
[
  AC_LIBSOURCES([file-type.c, file-type.h])
  AC_LIBOBJ([file-type])
])
