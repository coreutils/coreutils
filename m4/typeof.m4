#serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Jim Meyering.

AC_DEFUN([gl_TYPEOF],
[AC_CACHE_CHECK([for the __typeof__ operator], gl_cv_typeof,
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
    [[
      int i;
      __typeof__ i j;
    ]])],
    [gl_cv_typeof=yes],
    [gl_cv_typeof=no])
  ])
  if test $gl_cv_typeof = no; then
    AC_DEFINE(HAVE_TYPEOF, 1,
	      [Define to 1 if __typeof__ works with your compiler.])
  fi
])
