#serial 3
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_DUP2],
[
  AC_REPLACE_FUNCS(dup2)
  if test $ac_cv_func_dup2 = no; then
    gl_PREREQ_DUP2
  fi
])

# Prerequisites of lib/dup2.c.
AC_DEFUN([gl_PREREQ_DUP2], [
  AC_CHECK_HEADERS_ONCE(unistd.h)
])
