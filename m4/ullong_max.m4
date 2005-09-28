# ullong_max.m4 - define ULLONG_MAX if necessary

dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

AC_DEFUN([gl_ULLONG_MAX],
[
  dnl Avoid _AC_COMPUTE_INT-related macros, as they may not work with
  dnl types wider than long int, due to problems with expr.
  AC_CACHE_CHECK([whether ULONG_MAX < ULLONG_MAX],
    [gl_cv_ulong_max_lt_ullong_max],
    [AC_COMPILE_IFELSE(
      [AC_LANG_BOOL_COMPILE_TRY(
	 [AC_INCLUDES_DEFAULT],
	 [[(unsigned long int) -1 < (unsigned long long int) -1]])],
      [gl_cv_ulong_max_lt_ullong_max=yes],
      [gl_cv_ulong_max_lt_ullong_max=no])])
  if test $gl_cv_ulong_max_lt_ullong_max = yes; then
    AC_DEFINE([ULONG_MAX_LT_ULLONG_MAX], 1,
      [Define if ULONG_MAX < ULLONG_MAX, even if your compiler does not
       support ULLONG_MAX.])
  fi
])
