#serial 1002
# Experimental replacement for the function in the latest CVS autoconf.
# If the compile-test says strerror_r doesn't work, then resort to a
# `run'-test that works on BeOS and segfaults on DEC Unix.
# Use with the error.c file in ../lib.

undefine([AC_FUNC_STRERROR_R])

# AC_FUNC_STRERROR_R
# ------------------
AC_DEFUN([AC_FUNC_STRERROR_R],
[AC_CHECK_DECLS([strerror_r])
AC_CHECK_FUNCS([strerror_r])
if test $ac_cv_func_strerror_r = yes; then
  AC_CHECK_HEADERS(string.h)
  AC_CACHE_CHECK([for working strerror_r],
                 ac_cv_func_strerror_r_works,
   [
    AC_TRY_COMPILE(
     [
#       include <stdio.h>
#       if HAVE_STRING_H
#        include <string.h>
#       endif
     ],
     [
       char buf[100];
       char x = *strerror_r (0, buf, sizeof buf);
     ],
     ac_cv_func_strerror_r_works=yes,
     ac_cv_func_strerror_r_works=no
    )
    if test $ac_cv_func_strerror_r_works = no; then
      # strerror_r seems not to work, but now we have to choose between
      # systems that have relatively inaccessible declarations for the
      # function.  BeOS and DEC UNIX 4.0 fall in this category, but the
      # former has a strerror_r that returns char*, while the latter
      # has a strerror_r that returns `int'.
      # This test should segfault on the DEC system.
      AC_TRY_RUN(
       [
#       include <stdio.h>
#       include <string.h>
#       include <ctype.h>

	extern char *strerror_r ();

	int
	main ()
	{
	  char buf[100];
	  char x = *strerror_r (0, buf, sizeof buf);
	  exit (!isalpha (x));
	}
       ],
       ac_cv_func_strerror_r_works=yes,
       ac_cv_func_strerror_r_works=no,
       ac_cv_func_strerror_r_works=no)
    fi
  ])
  if test $ac_cv_func_strerror_r_works = yes; then
    AC_DEFINE_UNQUOTED(HAVE_WORKING_STRERROR_R, 1,
      [Define to 1 if `strerror_r' returns a string.])
  fi
fi
])# AC_FUNC_STRERROR_R
