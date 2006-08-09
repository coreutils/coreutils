# stdarg.m4 serial 1
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.
dnl Provide a working va_copy in combination with <stdarg.h>.

AC_DEFUN([gl_STDARG_H],
[
  AC_MSG_CHECKING([for va_copy])
  AC_CACHE_VAL([gl_cv_func_va_copy], [
    AC_TRY_COMPILE([#include <stdarg.h>], [
#ifndef va_copy
void (*func) (va_list, va_list) = va_copy;
#endif
],
      [gl_cv_func_va_copy=yes], [gl_cv_func_va_copy=no])])
  AC_MSG_RESULT([$gl_cv_func_va_copy])
  if test $gl_cv_func_va_copy = no; then
    # Provide a substitute, either __va_copy or as a simple assignment.
    AC_CACHE_VAL([gl_cv_func___va_copy], [
      AC_TRY_COMPILE([#include <stdarg.h>], [
#ifndef __va_copy
error, bail out
#endif
],
        [gl_cv_func___va_copy=yes], [gl_cv_func___va_copy=no])])
    if test $gl_cv_func___va_copy = yes; then
      AC_DEFINE([va_copy], [__va_copy],
        [Define as a macro for copying va_list variables.])
    else
      AH_VERBATIM([gl_VA_COPY], [/* A replacement for va_copy, if needed.  */
#define gl_va_copy(a,b) ((a) = (b))])
      AC_DEFINE([va_copy], [gl_va_copy],
        [Define as a macro for copying va_list variables.])
    fi
  fi
])
