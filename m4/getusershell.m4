# getusershell.m4 serial 3
dnl Copyright (C) 2002, 2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_GETUSERSHELL],
[
  AC_REPLACE_FUNCS(getusershell)
  if test $ac_cv_func_getusershell = no; then
    gl_PREREQ_GETUSERSHELL
  fi
])

# Prerequisites of lib/getusershell.c.
AC_DEFUN([gl_PREREQ_GETUSERSHELL], [
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_FUNCS_ONCE(isascii)
])
