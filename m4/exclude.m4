# exclude.m4 serial 5
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_EXCLUDE],
[
  AC_LIBSOURCES([exclude.c, exclude.h])
  AC_LIBOBJ([exclude])

  dnl Prerequisites of lib/exclude.c.
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_FUNCS_ONCE(isascii)
])
