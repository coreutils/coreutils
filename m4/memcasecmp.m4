#serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMCASECMP],
[
  AC_LIBSOURCES([memcasecmp.c, memcasecmp.h])
  AC_LIBOBJ([memcasecmp])

  dnl prerequisites
  AC_HEADER_STDC
  AC_CHECK_HEADERS([stddef.h])
  AC_C_CONST
  AC_TYPE_SIZE_T
])
