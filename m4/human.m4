#serial 9
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_HUMAN],
[
  AC_LIBSOURCES([human.c, human.h, intprops.h])
  AC_LIBOBJ([human])

  dnl Prerequisites of lib/human.h.
  AC_REQUIRE([AM_STDBOOL_H])
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])

  dnl Prerequisites of lib/human.c.
  :
])
