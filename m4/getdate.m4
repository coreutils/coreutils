# getdate.m4 serial 7
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETDATE],
[
  AC_LIBSOURCES([getdate.h, getdate.y])
  AC_LIBOBJ([getdate])

  dnl Prerequisites of lib/getdate.h.
  AC_REQUIRE([AM_STDBOOL_H])
  AC_REQUIRE([gl_TIMESPEC])

  dnl Prerequisites of lib/getdate.y.
  AC_REQUIRE([gl_BISON])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_FUNCS_ONCE(isascii)
  AC_STRUCT_TIMEZONE
  AC_REQUIRE([gl_CLOCK_TIME])
  AC_REQUIRE([gl_TM_GMTOFF])
])
