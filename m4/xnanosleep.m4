# xnanosleep.m4 serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Paul Eggert.

AC_DEFUN([gl_XNANOSLEEP],
[
  AC_LIBSOURCES([xnanosleep.c, xnanosleep.h])
  AC_LIBOBJ([xnanosleep])

  dnl Prerequisites of lib/xnanosleep.c.
  AC_REQUIRE([gl_PREREQ_GETHRXTIME])

  LIB_XNANOSLEEP=
  case $LIB_GETHRXTIME in
  ?*)
    AC_CACHE_CHECK([whether __linux__ is defined],
      gl_cv_have___linux__,
      [AC_EGREP_CPP([have___linux__],
	[
#        ifdef __linux__
	  have___linux__
#        endif
	],
	gl_cv_have___linux__=yes,
	gl_cv_have___linux__=no)])
    if test $gl_cv_have___linux__ = yes; then
      LIB_XNANOSLEEP=$LIB_GETHRXTIME
    fi;;
  esac
  AC_SUBST([LIB_XNANOSLEEP])
])
