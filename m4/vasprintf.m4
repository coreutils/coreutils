# vasprintf.m4 serial 1
dnl Copyright (C) 2002-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_VASPRINTF],
[
  AC_REPLACE_FUNCS(vasprintf)
  if test $ac_cv_func_vasprintf = no; then
    AC_LIBOBJ(asprintf)
    gl_PREREQ_VASPRINTF
    gl_PREREQ_ASPRINTF
  fi
])

# Prerequisites of lib/vasprintf.c.
AC_DEFUN([gl_PREREQ_VASPRINTF],
[
])

# Prerequisites of lib/asprintf.c.
AC_DEFUN([gl_PREREQ_ASPRINTF],
[
])
