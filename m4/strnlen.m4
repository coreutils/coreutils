# strnlen.m4 serial 4
dnl Copyright (C) 2002-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRNLEN],
[
  dnl Persuade glibc <string.h> to declare strnlen().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_FUNC_STRNLEN
  if test $ac_cv_func_strnlen_working = no; then
    # This is necessary because automake-1.6.1 doens't understand
    # that the above use of AC_FUNC_STRNLEN means we may have to use
    # lib/strnlen.c.
    #AC_LIBOBJ(strnlen)
    AC_DEFINE(strnlen, rpl_strnlen,
      [Define to rpl_strnlen if the replacement function should be used.])
    gl_PREREQ_STRNLEN
  fi
])

# Prerequisites of lib/strnlen.c.
AC_DEFUN([gl_PREREQ_STRNLEN], [:])
