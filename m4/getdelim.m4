# getdelim.m4 serial 1

dnl Copyright (C) 2005 Free Software dnl Foundation, Inc.
dnl
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_PREREQ(2.52)

AC_DEFUN([gl_FUNC_GETDELIM],
[
  AC_LIBSOURCES([getdelim.c, getdelim.h])

  dnl Persuade glibc <stdio.h> to declare getdelim().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REPLACE_FUNCS(getdelim)
  AC_CHECK_DECLS_ONCE(getdelim)

  if test $ac_cv_func_getdelim = no; then
    gl_PREREQ_GETDELIM
  fi
])

# Prerequisites of lib/getdelim.c.
AC_DEFUN([gl_PREREQ_GETDELIM],
[
  AC_CHECK_FUNCS([flockfile funlockfile])
])
