# filenamecat.m4 serial 7
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FILE_NAME_CONCAT],
[
  AC_LIBSOURCES([filenamecat.c, filenamecat.h])
  AC_LIBOBJ([filenamecat])

  dnl Prerequisites of lib/filenamecat.c.
  AC_CHECK_FUNCS_ONCE(mempcpy)
])
