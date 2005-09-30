#serial 6
# See if we need to use our replacement for Solaris' openat function.

dnl Copyright (C) 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([gl_FUNC_OPENAT],
[
  AC_LIBSOURCES([openat.c, openat.h, openat-die.c])
  AC_LIBOBJ([openat-die])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_FUNCS_ONCE([fdopendir])
  AC_REPLACE_FUNCS(openat)
  case $ac_cv_func_openat in
  yes) ;;
  *)
    AC_DEFINE([__OPENAT_PREFIX], [[rpl_]],
      [Define to rpl_ if the openat replacement function should be used.])
    gl_PREREQ_OPENAT;;
  esac
])

AC_DEFUN([gl_PREREQ_OPENAT],
[
  AC_REQUIRE([gl_SAVE_CWD])
])
