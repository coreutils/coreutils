# serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_LINEBUFFER],
[
  AC_LIBSOURCES([linebuffer.c, linebuffer.h])
  AC_LIBOBJ([linebuffer])

  dnl prerequisites
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([gl_FUNC_MEMSET])
])
