#serial 2

dnl autoconf tests required for use of mbswidth.c
dnl From Bruno Haible.

AC_DEFUN(jm_PREREQ_MBSWIDTH,
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AM_C_PROTOTYPES])
  AC_CHECK_HEADERS(limits.h stdlib.h string.h wchar.h wctype.h)
  AC_CHECK_FUNCS(isascii iswprint mbrtowc wcwidth)
  headers='
#     if HAVE_WCHAR_H
#      include <wchar.h>
#     endif
'
  AC_CHECK_DECLS([wcwidth], , , $headers)
  AC_MBSTATE_T
])
