#serial 3
dnl Copyright (C) 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CYCLE_CHECK],
[
  AC_LIBSOURCES([cycle-check.c, cycle-check.h])
  AC_LIBOBJ([cycle-check])

  dnl Prerequisites.
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([gl_STRUCT_DEV_INO])
  AC_REQUIRE([gl_SAME_INODE])
])
