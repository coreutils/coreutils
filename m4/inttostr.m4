#serial 5
dnl Copyright (C) 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_INTTOSTR],
[
  AC_LIBSOURCES([inttostr.c, inttostr.h, intprops.h])

  dnl We don't technically need to list the following .c files, since their
  dnl functions are named in the AC_LIBOBJ calls, but this is an unusual
  dnl module and it seems a little clearer to do so.
  AC_LIBSOURCES([imaxtostr.c, offtostr.c, umaxtostr.c])

  AC_LIBOBJ([imaxtostr])
  AC_LIBOBJ([offtostr])
  AC_LIBOBJ([umaxtostr])

  gl_PREREQ_INTTOSTR
  gl_PREREQ_IMAXTOSTR
  gl_PREREQ_OFFTOSTR
  gl_PREREQ_UMAXTOSTR
])

# Prerequisites of lib/inttostr.h.
AC_DEFUN([gl_PREREQ_INTTOSTR], [
  AC_REQUIRE([gl_AC_TYPE_INTMAX_T])
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([AC_TYPE_OFF_T])
  :
])

# Prerequisites of lib/imaxtostr.c.
AC_DEFUN([gl_PREREQ_IMAXTOSTR], [:])

# Prerequisites of lib/offtostr.c.
AC_DEFUN([gl_PREREQ_OFFTOSTR], [:])

# Prerequisites of lib/umaxtostr.c.
AC_DEFUN([gl_PREREQ_UMAXTOSTR], [:])
