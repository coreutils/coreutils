# obstack.m4 serial 4
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_OBSTACK],
[
  AC_FUNC_OBSTACK
  dnl Note: AC_FUNC_OBSTACK does AC_LIBSOURCES([obstack.h, obstack.c]).
  if test $ac_cv_func_obstack = no; then
    gl_PREREQ_OBSTACK
  fi
])

# Prerequisites of lib/obstack.c.
AC_DEFUN([gl_PREREQ_OBSTACK],
[
  AC_REQUIRE([gl_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([gl_AC_HEADER_STDINT_H])
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  :
])
