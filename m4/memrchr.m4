# memrchr.m4 serial 4
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMRCHR],
[
  AC_LIBSOURCES([memrchr.c, memrchr.h])

  dnl Persuade glibc <string.h> to declare memrchr().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_CHECK_DECLS_ONCE([memrchr])

  AC_REPLACE_FUNCS(memrchr)
  if test $ac_cv_func_memrchr = no; then
    gl_PREREQ_MEMRCHR
  fi
])

# Prerequisites of lib/memrchr.c.
AC_DEFUN([gl_PREREQ_MEMRCHR], [:])
