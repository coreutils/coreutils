#serial 6
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SAVE_CWD],
[
  AC_LIBSOURCES([save-cwd.c, save-cwd.h])
  AC_LIBOBJ([save-cwd])
  dnl Prerequisites for lib/save-cwd.c.
  AC_CHECK_FUNCS_ONCE(fchdir)
  AC_CHECK_HEADERS_ONCE(unistd.h)
])
