#serial 1000
# Experimental replacement for the function in the latest CVS autoconf.
# Use with the error.c file in ../lib.

undefine([AC_FUNC_STRERROR_R])

# AC_FUNC_STRERROR_R
# ------------------
AC_DEFUN([AC_FUNC_STRERROR_R],
[# Check strerror_r
AC_CHECK_FUNCS([strerror_r])
if test $ac_cv_func_strerror_r = yes; then
  AC_CHECK_HEADERS(string.h)
  AC_CHECK_DECLS([strerror_r])
  AC_CACHE_CHECK([for working strerror_r],
                 ac_cv_func_strerror_r_works,
   [
    AC_TRY_COMPILE(
     [
#       include <stdio.h>
#       if HAVE_STRING_H
#        include <string.h>
#       endif
#ifndef HAVE_DECL_STRERROR_R
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRERROR_R
char *strerror_r ();
#endif
     ],
     [
       char buf;
       char x = *strerror_r (0, &buf, sizeof buf);
     ],
     ac_cv_func_strerror_r_works=yes,
     ac_cv_func_strerror_r_works=no
    )
    if test $ac_cv_func_strerror_r_works = yes; then
      AC_DEFINE_UNQUOTED(HAVE_WORKING_STRERROR_R, 1,
        [Define to 1 if `strerror_r' returns a string.])
    fi
  ])
fi
])# AC_FUNC_STRERROR_R
