# safe-write.m4 serial 2
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAFE_WRITE],
[
  AC_LIBSOURCES([safe-write.c, safe-write.h])
  AC_LIBOBJ([safe-write])

  gl_PREREQ_SAFE_WRITE
])

# Prerequisites of lib/safe-write.c.
AC_DEFUN([gl_PREREQ_SAFE_WRITE],
[
  gl_PREREQ_SAFE_READ
])
