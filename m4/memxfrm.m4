dnl Copyright (C) 2006, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MEMXFRM],
[
  AC_LIBSOURCES([memxfrm.c, memxfrm.h])
  AC_LIBOBJ([memxfrm])

  AC_REQUIRE([AC_C_RESTRICT])

  dnl Prerequisites of lib/memcoll.c.
  AC_CHECK_FUNCS_ONCE([strxfrm])
])
