# canon-host.m4 serial 7
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CANON_HOST],
[
  AC_LIBSOURCES([canon-host.c, canon-host.h])
  AC_LIBOBJ([canon-host])
  gl_PREREQ_CANON_HOST
])

AC_DEFUN([gl_PREREQ_CANON_HOST], [
  AC_REQUIRE([gl_GETADDRINFO])
])
