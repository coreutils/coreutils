# strcase.m4 serial 1
dnl Copyright (C) 2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STRCASE],
[
  gl_FUNC_STRCASECMP
  gl_FUNC_STRNCASECMP
])

AC_DEFUN([gl_FUNC_STRCASECMP],
[
  AC_REPLACE_FUNCS(strcasecmp)
  if test $ac_cv_func_strcasecmp = no; then
    gl_PREREQ_STRCASECMP
  fi
])

AC_DEFUN([gl_FUNC_STRNCASECMP],
[
  AC_REPLACE_FUNCS(strncasecmp)
  if test $ac_cv_func_strncasecmp = no; then
    gl_PREREQ_STRNCASECMP
  fi
])

# Prerequisites of lib/strcasecmp.c.
AC_DEFUN([gl_PREREQ_STRCASECMP], [
  :
])

# Prerequisites of lib/strncasecmp.c.
AC_DEFUN([gl_PREREQ_STRNCASECMP], [
  :
])
