# pathmax.m4 serial 4
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PATHMAX],
[
  AC_LIBSOURCES([pathmax.h])

  dnl Prerequisites of lib/pathmax.h.
  AC_CHECK_HEADERS_ONCE(sys/param.h unistd.h)
])
