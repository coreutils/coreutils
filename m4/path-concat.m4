# path-concat.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PATH_CONCAT],
[
  AC_LIBSOURCES([path-concat.c, path-concat.h])
  AC_LIBOBJ([path-concat])

  dnl Prerequisites of lib/path-concat.c.
  AC_CHECK_FUNCS_ONCE(mempcpy)
])
