# getcwd.m4 - check for working getcwd that is compatible with glibc

# Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Written by Paul Eggert.

AC_DEFUN([gl_FUNC_GETCWD_NULL],
  [
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
  AC_CHECK_HEADERS_ONCE(fcntl.h)
  :
])
