# strverscmp.m4 serial 3
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRVERSCMP],
[
  AC_LIBSOURCES([strverscmp.c, strverscmp.h])

  dnl Persuade glibc <string.h> to declare strverscmp().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REPLACE_FUNCS(strverscmp)
  if test $ac_cv_func_strverscmp = no; then
    gl_PREREQ_STRVERSCMP
  fi
])

# Prerequisites of lib/strverscmp.c.
AC_DEFUN([gl_PREREQ_STRVERSCMP], [
  :
])
