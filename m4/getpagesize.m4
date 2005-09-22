# getpagesize.m4 serial 5
dnl Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETPAGESIZE],
[
  AC_LIBSOURCES([getpagesize.h])

  dnl Prerequisites of lib/getpagesize.h.
  AC_CHECK_HEADERS_ONCE(sys/param.h)
  AC_CHECK_HEADERS(OS.h)
  AC_CHECK_FUNCS(getpagesize)
])
