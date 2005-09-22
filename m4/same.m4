# same.m4 serial 6
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAME],
[
  AC_LIBSOURCES([same.c, same.h])
  AC_LIBOBJ([same])

  dnl Prerequisites of lib/same.c.
  AC_REQUIRE([AC_SYS_LONG_FILE_NAMES])
  AC_CHECK_FUNCS_ONCE([pathconf])
])
