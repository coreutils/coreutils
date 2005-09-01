#serial 8

dnl Copyright (C) 1998, 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.
dnl Provide lchown on systems that lack it.

AC_DEFUN([gl_FUNC_LCHOWN],
[
  AC_LIBSOURCES([lchown.c, lchown.h])
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_REQUIRE([gl_FUNC_CHOWN])
  AC_REQUIRE([gl_STAT_MACROS])
  AC_REPLACE_FUNCS(lchown)
])
