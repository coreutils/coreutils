# mbswidth.m4 serial 11
dnl Copyright (C) 2000-2002, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl autoconf tests required for use of mbswidth.c
dnl From Bruno Haible.

AC_DEFUN([gl_MBSWIDTH],
[
  AC_CHECK_HEADERS_ONCE(wchar.h wctype.h)
  AC_CHECK_FUNCS_ONCE(isascii iswprint mbsinit)
  AC_CHECK_FUNCS(iswcntrl wcwidth)
  gl_FUNC_MBRTOWC

  AC_CACHE_CHECK([whether wcwidth is declared], ac_cv_have_decl_wcwidth,
    [AC_TRY_COMPILE([
/* AIX 3.2.5 declares wcwidth in <string.h>. */
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_WCHAR_H
# include <wchar.h>
#endif
], [
#ifndef wcwidth
  char *p = (char *) wcwidth;
#endif
], ac_cv_have_decl_wcwidth=yes, ac_cv_have_decl_wcwidth=no)])
  if test $ac_cv_have_decl_wcwidth = yes; then
    ac_val=1
  else
    ac_val=0
  fi
  AC_DEFINE_UNQUOTED(HAVE_DECL_WCWIDTH, $ac_val,
    [Define to 1 if you have the declaration of wcwidth(), and to 0 otherwise.])

  dnl UnixWare 7.1.1 <wchar.h> has a declaration of a function mbswidth()
  dnl that clashes with ours.
  AC_CACHE_CHECK([whether mbswidth is declared in <wchar.h>],
    ac_cv_have_decl_mbswidth,
    [AC_TRY_COMPILE([
#if HAVE_WCHAR_H
# include <wchar.h>
#endif
], [
  char *p = (char *) mbswidth;
], ac_cv_have_decl_mbswidth=yes, ac_cv_have_decl_mbswidth=no)])
  if test $ac_cv_have_decl_mbswidth = yes; then
    ac_val=1
  else
    ac_val=0
  fi
  AC_DEFINE_UNQUOTED(HAVE_DECL_MBSWIDTH_IN_WCHAR_H, $ac_val,
    [Define to 1 if you have a declaration of mbswidth() in <wchar.h>, and to 0 otherwise.])

  AC_TYPE_MBSTATE_T
])
