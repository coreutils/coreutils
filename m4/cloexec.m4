#serial 5
dnl Copyright (C) 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CLOEXEC],
[
  AC_LIBSOURCES([cloexec.c, cloexec.h])
  AC_LIBOBJ([cloexec])
])
