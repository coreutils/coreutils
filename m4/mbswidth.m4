#serial 1

dnl autoconf tests required for use of mbswidth.c
dnl From Bruno Haible.

AC_DEFUN(jm_PREREQ_MBSWIDTH,
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AM_C_PROTOTYPES])
  AC_CHECK_HEADERS(limits.h stdlib.h string.h wchar.h wctype.h)
  AC_CHECK_FUNCS(isascii iswprint mbrtowc wcwidth)
  AC_MBSTATE_T
])
