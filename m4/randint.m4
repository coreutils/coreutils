dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_RANDINT],
[
  AC_LIBSOURCES([randint.c, randint.h])
  AC_LIBOBJ([randint])

  AC_REQUIRE([AC_C_INLINE])
])
