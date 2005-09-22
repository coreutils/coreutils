# mkdir-p.m4 serial 9
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MKDIR_PARENTS],
[
  AC_LIBSOURCES([mkdir-p.c, mkdir-p.h])
  AC_LIBOBJ([mkdir-p])

  dnl Prerequisites of lib/mkdir-p.c.
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([gl_AFS])
])
