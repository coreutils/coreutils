# getcwd.m4 - check for working getcwd that is compatible with glibc

# Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

AC_DEFUN([gl_FUNC_GETCWD_NULL],
  [
   AC_LIBSOURCES([getcwd.c, getcwd.h])

   AC_CHECK_HEADERS_ONCE(unistd.h)
   AC_CACHE_CHECK([whether getcwd (NULL, 0) allocates memory for result],
     [gl_cv_func_getcwd_null],
     [AC_TRY_RUN(
        [
#	 include <stdlib.h>
#	 ifdef HAVE_UNISTD_H
#	  include <unistd.h>
#	 endif
#	 ifndef getcwd
	 char *getcwd ();
#	 endif
	 int
	 main ()
	 {
	   if (chdir ("/") != 0)
	     exit (1);
	   else
	     {
	       char *f = getcwd (NULL, 0);
	       exit (! (f && f[0] == '/' && !f[1]));
	     }
	 }],
	[gl_cv_func_getcwd_null=yes],
	[gl_cv_func_getcwd_null=no],
	[gl_cv_func_getcwd_null=no])])
])

AC_DEFUN([gl_FUNC_GETCWD],
[
  AC_REQUIRE([gl_FUNC_GETCWD_NULL])

  case $gl_cv_func_getcwd_null in
  yes) gl_FUNC_GETCWD_PATH_MAX;;
  esac

  case $gl_cv_func_getcwd_null,$gl_cv_func_getcwd_path_max in
  yes,yes) ;;
  *)
    AC_LIBOBJ([getcwd])
    AC_DEFINE([__GETCWD_PREFIX], [[rpl_]],
      [Define to rpl_ if the getcwd replacement function should be used.])
    gl_PREREQ_GETCWD;;
  esac
])

# Prerequisites of lib/getcwd.c.
AC_DEFUN([gl_PREREQ_GETCWD],
[
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_DIRENT_D_INO])
  :
])
