# gethrxtime.m4 serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETHRXTIME],
[
  AC_LIBSOURCES([gethrxtime.c, gethrxtime.h, xtime.h])
  AC_REQUIRE([gl_ARITHMETIC_HRTIME_T])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_REQUIRE([gl_XTIME])
  AC_CHECK_DECLS([gethrtime], [], [], [#include <time.h>])
  case $ac_cv_have_decl_gethrtime,$gl_cv_arithmetic_hrtime_t in
  yes,yes) ;;
  *)
    AC_LIBOBJ([gethrxtime])
    gl_PREREQ_GETHRXTIME;;
  esac
])

# Test whether hrtime_t is an arithmetic type.
# It is not arithmetic in older Solaris c89 (which insists on
# not having a long long int type).
AC_DEFUN([gl_ARITHMETIC_HRTIME_T],
[
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CACHE_CHECK([for arithmetic hrtime_t], gl_cv_arithmetic_hrtime_t,
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([#include <time.h>],
	  [hrtime_t x = 0; return x/x;])],
       [gl_cv_arithmetic_hrtime_t=yes],
       [gl_cv_arithmetic_hrtime_t=no])])
  if test $gl_cv_arithmetic_hrtime_t = yes; then
    AC_DEFINE([HAVE_ARITHMETIC_HRTIME_T], 1,
      [Define if you have an arithmetic hrtime_t type.])
  fi
])

# Prerequisites of lib/xtime.h.
AC_DEFUN([gl_XTIME],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
  :
])

# Prerequisites of lib/gethrxtime.c.
AC_DEFUN([gl_PREREQ_GETHRXTIME],
[
  dnl Do not AC_REQUIRE([gl_CLOCK_TIME]), since that would unnecessarily
  dnl require -lrt on Solaris.  Invocations of clock_gettime should be
  dnl safe in gethrxtime.c since Solaris has native gethrtime.
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CHECK_FUNCS_ONCE(gettimeofday microuptime nanouptime)
  :
])
