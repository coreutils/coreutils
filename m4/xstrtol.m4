#serial 7
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_XSTRTOL],
[
  AC_LIBSOURCES([xstrtol.c, xstrtol.h, xstrtoul.c, intprops.h])
  AC_LIBOBJ([xstrtol])
  AC_LIBOBJ([xstrtoul])

  AC_REQUIRE([gl_PREREQ_XSTRTOL])
  AC_REQUIRE([gl_PREREQ_XSTRTOUL])
])

# Prerequisites of lib/xstrtol.h.
AC_DEFUN([gl_PREREQ_XSTRTOL_H],
[
  AC_REQUIRE([gl_AC_TYPE_INTMAX_T])
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
])

# Prerequisites of lib/xstrtol.c.
AC_DEFUN([gl_PREREQ_XSTRTOL],
[
  AC_REQUIRE([gl_PREREQ_XSTRTOL_H])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_FUNCS_ONCE(isascii)
  AC_CHECK_DECLS([strtoimax, strtoumax])
])

# Prerequisites of lib/xstrtoul.c.
AC_DEFUN([gl_PREREQ_XSTRTOUL],
[
  AC_REQUIRE([gl_PREREQ_XSTRTOL])
])
