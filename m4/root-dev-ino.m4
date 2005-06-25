#serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_ROOT_DEV_INO],
[
  AC_LIBSOURCES([root-dev-ino.c, root-dev-ino.h, dev-ino.h])
  AC_LIBOBJ([root-dev-ino])

  dnl Prerequisites
  AC_REQUIRE([AC_FUNC_LSTAT])
  :
])
