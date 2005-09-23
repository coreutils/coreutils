dnl Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_UTIMENS],
[
  AC_LIBSOURCES([utimens.c, utimens.h])
  AC_LIBOBJ([utimens])

  dnl Prerequisites of lib/utimens.c.
  AC_REQUIRE([gl_TIMESPEC])
  AC_REQUIRE([gl_FUNC_UTIMES])
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_TIMESPEC])
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_UTIMBUF])
  AC_CHECK_FUNCS_ONCE([futimes futimesat])
])
