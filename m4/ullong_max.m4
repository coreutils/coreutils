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
  AC_CACHE_CHECK([for ULLONG_MAX], gl_cv_ullong_max,
    [gl_cv_ullong_max=no
     AC_EGREP_CPP([ULLONG_MAX is defined],
       [
	#include <limits.h>
	#ifdef ULLONG_MAX
	 "ULLONG_MAX is defined"
	#endif
       ],
       [gl_cv_ullong_max=yes])
     case $gl_cv_ullong_max in
     no)
       for gl_expr in \
	 18446744073709551615ULL \
	 4722366482869645213695ULL \
	 340282366920938463463374607431768211455ULL
       do
	 AC_TRY_COMPILE([],
	   [char test[$gl_expr == (unsigned long long int) -1 ? 1 : -1];
	    static unsigned long long int i = $gl_expr;
	    return i && test;],
	   [gl_cv_ullong_max=$gl_expr])
         test $gl_cv_ullong_max != no && break
       done
     esac])
  case $gl_cv_ullong_max in
  yes | no) ;;
  *)
    AC_DEFINE_UNQUOTED([ULLONG_MAX], [$gl_cv_ullong_max],
      [Define as the maximum value of the type 'unsigned long long int',
       if the system doesn't define it, and if the system has that type.]);;
  esac
])
