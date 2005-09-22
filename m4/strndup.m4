# strndup.m4 serial 5
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRNDUP],
[
  AC_LIBSOURCES([strndup.c, strndup.h])

  dnl Persuade glibc <string.h> to declare strndup().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REPLACE_FUNCS(strndup)
  if test $ac_cv_func_strndup = no; then
    gl_PREREQ_STRNDUP
  fi
])

# Prerequisites of lib/strndup.c.
AC_DEFUN([gl_PREREQ_STRNDUP], [:])
