#serial 2

dnl From Jim Meyering.
dnl Determine whether the strerror_r implementation is one of
dnl the broken ones that returns `int' rather than `char*'.
dnl Actually, this tests only whether it returns a scalar
dnl or an array, but that should be enough.
dnl On at least DEC UNIX 4.0[A-D] and HP-UX B.10.20, strerror_r
dnl returns `int'.  This is used by lib/error.c.

AC_DEFUN(jm_FUNC_STRERROR_R,
[
  # Check strerror_r
  AC_CHECK_FUNCS([strerror_r])
  if test $ac_cv_func_strerror_r = yes; then
    AC_CHECK_HEADERS(string.h)
    AC_CACHE_CHECK([for working strerror_r],
                   jm_cv_func_working_strerror_r,
     [
      AC_TRY_COMPILE(
       [
#       include <stdio.h>
#       if HAVE_STRING_H
#        include <string.h>
#       endif
       ],
       [
	 int buf; /* avoiding square brackets makes this easier */
	 char x = *strerror_r (0, buf, sizeof buf);
       ],
       jm_cv_func_working_strerror_r=yes,
       jm_cv_func_working_strerror_r=no
      )
      if test $jm_cv_func_working_strerror_r = yes; then
	AC_DEFINE_UNQUOTED(HAVE_WORKING_STRERROR_R, 1,
	  [Define to 1 if strerror_r returns a string.])
      fi
    ])
  fi
])
