# savedir.m4 serial 4
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAVEDIR],
[
  AC_LIBSOURCES([savedir.c, savedir.h])
  AC_LIBOBJ([savedir])

  dnl Prerequisites of lib/savedir.c.
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_REQUIRE([AC_FUNC_CLOSEDIR_VOID])
])
