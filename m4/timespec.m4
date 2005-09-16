#serial 11

# Copyright (C) 2000, 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering

AC_DEFUN([gl_TIMESPEC],
[
  AC_LIBSOURCES([timespec.h])

  dnl Prerequisites of lib/timespec.h.
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CHECK_HEADERS_ONCE(sys/time.h)
  gl_CHECK_TYPE_STRUCT_TIMESPEC

  dnl Persuade glibc <time.h> to declare nanosleep().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_CHECK_DECLS(nanosleep, , , [#include <time.h>])
])

dnl Define HAVE_STRUCT_TIMESPEC if `struct timespec' is declared
dnl in time.h or sys/time.h.

AC_DEFUN([gl_CHECK_TYPE_STRUCT_TIMESPEC],
[
  dnl Persuade pedantic Solaris to declare struct timespec.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([AC_HEADER_TIME])
  AC_CHECK_HEADERS_ONCE(sys/time.h)
  AC_CACHE_CHECK([for struct timespec], fu_cv_sys_struct_timespec,
    [AC_TRY_COMPILE(
      [
#      if TIME_WITH_SYS_TIME
#       include <sys/time.h>
#       include <time.h>
#      else
#       if HAVE_SYS_TIME_H
#        include <sys/time.h>
#       else
#        include <time.h>
#       endif
#      endif
      ],
      [static struct timespec x; x.tv_sec = x.tv_nsec;],
      fu_cv_sys_struct_timespec=yes,
      fu_cv_sys_struct_timespec=no)
    ])

  if test $fu_cv_sys_struct_timespec = yes; then
    AC_DEFINE(HAVE_STRUCT_TIMESPEC, 1,
	      [Define if struct timespec is declared in <time.h>. ])
  fi
])
