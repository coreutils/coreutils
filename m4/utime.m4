#serial 7

dnl From Jim Meyering
dnl Replace the utime function on systems that need it.

# Copyright (C) 1998, 2000, 2001, 2003, 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl FIXME

AC_DEFUN([gl_FUNC_UTIME],
[
  AC_REQUIRE([AC_FUNC_UTIME_NULL])
  if test $ac_cv_func_utime_null = no; then
    AC_LIBOBJ(utime)
    AC_DEFINE(utime, rpl_utime,
      [Define to rpl_utime if the replacement function should be used.])
    gl_PREREQ_UTIME
  fi
])

# Prerequisites of lib/utime.c.
AC_DEFUN([gl_PREREQ_UTIME],
[
  AC_CHECK_HEADERS_ONCE(utime.h)
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_UTIMBUF])
  gl_FUNC_UTIMES_NULL
])
