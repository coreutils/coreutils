# mbrtowc.m4 serial 7
dnl Copyright (C) 2001-2002, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert

dnl This file can be removed, and gl_FUNC_MBRTOWC replaced with
dnl AC_FUNC_MBRTOWC, when autoconf 2.57 can be assumed everywhere.

AC_DEFUN([gl_FUNC_MBRTOWC],
[
  AC_CACHE_CHECK([whether mbrtowc and mbstate_t are properly declared],
    gl_cv_func_mbrtowc,
    [AC_TRY_LINK(
       [#include <wchar.h>],
       [mbstate_t state; return ! (sizeof state && mbrtowc);],
       gl_cv_func_mbrtowc=yes,
       gl_cv_func_mbrtowc=no)])
  if test $gl_cv_func_mbrtowc = yes; then
    AC_DEFINE(HAVE_MBRTOWC, 1,
      [Define to 1 if mbrtowc and mbstate_t are properly declared.])
  fi
])
