# memcmp.m4 serial 11
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMCMP],
[
  AC_REQUIRE([AC_FUNC_MEMCMP])
  if test $ac_cv_func_memcmp_working = no; then
    AC_DEFINE(memcmp, rpl_memcmp,
      [Define to rpl_memcmp if the replacement function should be used.])
    gl_PREREQ_MEMCMP
  fi
])

# Prerequisites of lib/memcmp.c.
AC_DEFUN([gl_PREREQ_MEMCMP], [:])
