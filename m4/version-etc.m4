#serial 1
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_VERSION_ETC],
[
  dnl FIXME: if/when this becomes a gnulib macro, make version-etc-fsf.c
  dnl be merely `suggested' or `recommended', not required.
  AC_LIBSOURCES([version-etc.c, version-etc.h, version-etc-fsf.c])
  AC_LIBOBJ([version-etc-fsf])
  AC_LIBOBJ([version-etc])

  dnl Prerequisites.
  :
])
