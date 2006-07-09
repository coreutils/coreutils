# savedir.m4 serial 7
dnl Copyright (C) 2002, 2003, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAVEDIR],
[
  AC_LIBSOURCES([savedir.c, savedir.h])
  AC_LIBOBJ([savedir])

  dnl Prerequisites of lib/savedir.c.
  AC_CHECK_HEADERS_ONCE([dirent.h])dnl
])
