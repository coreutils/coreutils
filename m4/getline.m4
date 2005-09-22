# getline.m4 serial 13

dnl Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2005 Free Software
dnl Foundation, Inc.
dnl
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_PREREQ(2.52)

dnl See if there's a working, system-supplied version of the getline function.
dnl We can't just do AC_REPLACE_FUNCS(getline) because some systems
dnl have a function by that name in -linet that doesn't have anything
dnl to do with the function we need.
AC_DEFUN([gl_FUNC_GETLINE],
[
  AC_LIBSOURCES([getline.c, getline.h])

  dnl Persuade glibc <stdio.h> to declare getline().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_CHECK_DECLS([getline])

  gl_getline_needs_run_time_check=no
  AC_CHECK_FUNC(getline,
		dnl Found it in some library.  Verify that it works.
		gl_getline_needs_run_time_check=yes,
		am_cv_func_working_getline=no)
  if test $gl_getline_needs_run_time_check = yes; then
    AC_CACHE_CHECK([for working getline function], am_cv_func_working_getline,
    [echo fooN |tr -d '\012'|tr N '\012' > conftest.data
    AC_TRY_RUN([
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
    int main ()
    { /* Based on a test program from Karl Heuer.  */
      char *line = NULL;
      size_t siz = 0;
      int len;
      FILE *in = fopen ("./conftest.data", "r");
      if (!in)
	return 1;
      len = getline (&line, &siz, in);
      exit ((len == 4 && line && strcmp (line, "foo\n") == 0) ? 0 : 1);
    }
    ], am_cv_func_working_getline=yes dnl The library version works.
    , am_cv_func_working_getline=no dnl The library version does NOT work.
    , am_cv_func_working_getline=no dnl We're cross compiling.
    )])
  fi

  if test $am_cv_func_working_getline = no; then
    dnl We must choose a different name for our function, since on ELF systems
    dnl a broken getline() in libc.so would override our getline() in
    dnl libgettextlib.so.
    AC_DEFINE([getline], [gnu_getline],
      [Define to a replacement function name for getline().])
    AC_LIBOBJ(getline)

    gl_PREREQ_GETLINE
  fi
])

# Prerequisites of lib/getline.c.
AC_DEFUN([gl_PREREQ_GETLINE],
[
  gl_FUNC_GETDELIM
])
