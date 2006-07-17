# mkdir-p.m4 serial 11
dnl Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MKDIR_PARENTS],
[
  AC_LIBSOURCES([dirchownmod.c, dirchownmod.h, mkdir-p.c, mkdir-p.h])
  AC_LIBOBJ([dirchownmod])
  AC_LIBOBJ([mkdir-p])

  dnl Prerequisites of lib/dirchownmod.c.
  AC_REQUIRE([gl_FUNC_LCHMOD])
  AC_REQUIRE([gl_FUNC_LCHOWN])
])
