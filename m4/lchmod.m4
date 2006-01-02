#serial 1

dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.
dnl Provide a replacement for lchmod on hosts that lack it.

AC_DEFUN([gl_FUNC_LCHMOD],
[
  AC_LIBSOURCES([lchmod.h])
  AC_CHECK_FUNCS_ONCE([lchmod])
])
