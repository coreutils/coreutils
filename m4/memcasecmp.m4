#serial 3
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MEMCASECMP],
[
  AC_LIBSOURCES([memcasecmp.c, memcasecmp.h])
  AC_LIBOBJ([memcasecmp])

  dnl Prerequisites of lib/memcasecmp.c.
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_FUNCS_ONCE(isascii)
])
