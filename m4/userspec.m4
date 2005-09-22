#serial 8
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_USERSPEC],
[
  AC_LIBSOURCES([userspec.c, userspec.h, intprops.h])
  AC_LIBOBJ([userspec])

  dnl Prerequisites of lib/userspec.c.
  AC_CHECK_HEADERS_ONCE(sys/param.h)
])
