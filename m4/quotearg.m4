# quotearg.m4 serial 4
dnl Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_QUOTEARG],
[
  AC_LIBSOURCES([quotearg.c, quotearg.h])
  AC_LIBOBJ([quotearg])

  dnl Prerequisites of lib/quotearg.c.
  AC_CHECK_HEADERS_ONCE(wchar.h wctype.h)
  AC_CHECK_FUNCS_ONCE(iswprint mbsinit)
  AC_TYPE_MBSTATE_T
  gl_FUNC_MBRTOWC
])
