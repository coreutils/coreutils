#serial 7

dnl autoconf tests required for use of mbswidth.c
dnl From Bruno Haible.

AC_DEFUN([jm_PREREQ_MBSWIDTH],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(limits.h stdlib.h string.h wchar.h wctype.h)
  AC_CHECK_FUNCS(isascii iswcntrl iswprint mbsinit wcwidth)
  jm_FUNC_MBRTOWC

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

  AC_TYPE_MBSTATE_T
])
