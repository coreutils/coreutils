# memcoll.m4 serial 4
dnl Copyright (C) 2002, 2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MEMCOLL],
[
  dnl Prerequisites of lib/memcoll.c.
  AC_REQUIRE([AC_FUNC_MEMCMP])
  AC_FUNC_STRCOLL
])
