# dirname.m4 serial 5
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_DIRNAME],
[
  AC_LIBSOURCES([dirname.c, dirname.h])
  AC_LIBOBJ([dirname])

  dnl Prerequisites of lib/dirname.h.
  AC_REQUIRE([gl_AC_DOS])

  dnl No prerequisites of lib/basename.c, lib/dirname.c, lib/stripslash.c.
])
