# stdint.m4 serial 5
dnl Copyright (C) 2001-2002, 2004-2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.
dnl Test whether <stdint.h> is supported or must be substituted.

AC_DEFUN([gl_STDINT_H],
[dnl Check for <inttypes.h>.
AC_REQUIRE([gt_HEADER_INTTYPES_H])
dnl Check for <sys/inttypes.h>.
AC_CHECK_HEADERS([sys/inttypes.h])
dnl Check for <sys/bitypes.h> (used in Linux libc4 >= 4.6.7 and libc5).
AC_CHECK_HEADERS([sys/bitypes.h])

AC_MSG_CHECKING([for stdint.h])
AC_CACHE_VAL(gl_cv_header_stdint_h, [
  AC_TRY_COMPILE([#include <stdint.h>], [],
    gl_cv_header_stdint_h=yes, gl_cv_header_stdint_h=no)])
AC_MSG_RESULT([$gl_cv_header_stdint_h])
if test $gl_cv_header_stdint_h = yes; then
  AC_DEFINE(HAVE_STDINT_H, 1,
            [Define if you have a working <stdint.h> header file.])
  STDINT_H=''
else
  STDINT_H='stdint.h'

  dnl Is long == int64_t ?
  AC_CACHE_CHECK([whether 'long' is 64 bit wide], gl_cv_long_bitsize_64, [
    AC_TRY_COMPILE([
#define POW63  ((((((long) 1 << 15) << 15) << 15) << 15) << 3)
#define POW64  ((((((long) 1 << 15) << 15) << 15) << 15) << 4)
typedef int array [2 * (POW63 != 0 && POW64 == 0) - 1];
], , gl_cv_long_bitsize_64=yes, gl_cv_long_bitsize_64=no)])
  if test $gl_cv_long_bitsize_64 = yes; then
    HAVE_LONG_64BIT=1
  else
    HAVE_LONG_64BIT=0
  fi
  AC_SUBST(HAVE_LONG_64BIT)

  dnl Is long long == int64_t ?
  AC_CACHE_CHECK([whether 'long long' is 64 bit wide], gl_cv_longlong_bitsize_64, [
    AC_TRY_COMPILE([
#define POW63  ((((((long long) 1 << 15) << 15) << 15) << 15) << 3)
#define POW64  ((((((long long) 1 << 15) << 15) << 15) << 15) << 4)
typedef int array [2 * (POW63 != 0 && POW64 == 0) - 1];
], , gl_cv_longlong_bitsize_64=yes, gl_cv_longlong_bitsize_64=no)])
  if test $gl_cv_longlong_bitsize_64 = yes; then
    HAVE_LONG_LONG_64BIT=1
  else
    HAVE_LONG_LONG_64BIT=0
  fi
  AC_SUBST(HAVE_LONG_LONG_64BIT)

fi
AC_SUBST(STDINT_H)
])
