# hash-pjw.m4 serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_HASH_PJW],
[
  AC_LIBSOURCES([hash-pjw.c, hash-pjw.h])
  AC_LIBOBJ([hash-pjw])

  dnl prerequisites
  dnl size_t and const; those aren't really worth mentioning anymore
])
