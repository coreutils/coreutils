# c-strtod.m4 serial 4

# Copyright (C) 2004 Free Software Foundation, Inc.

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

AC_DEFUN([gl_C99_STRTOLD],
[
  AC_CACHE_CHECK([whether strtold conforms to C99],
    [gl_cv_func_c99_strtold],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[/* On HP-UX before 11.23, strtold returns a struct instead of
		long double.  Reject implementations like that, by requiring
		compatibility with the C99 prototype.  */
	     #include <stdlib.h>
	     static long double (*p) (char const *, char **) = strtold;
	     static long double
	     test (char const *nptr, char **endptr)
	     {
	       long double r;
	       r = strtold (nptr, endptr);
	       return r;
	     }]],
           [[return test ("1.0", NULL) != 1 || p ("1.0", NULL) != 1;]])],
       [gl_cv_func_c99_strtold=yes],
       [gl_cv_func_c99_strtold=no])])
  if test $gl_cv_func_c99_strtold = yes; then
    AC_DEFINE([HAVE_C99_STRTOLD], 1, [Define to 1 if strtold conforms to C99.])
  fi
])

AC_DEFUN([gl_C_STRTOD],
[
  dnl Prerequisites of lib/c-strtod.c.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  :
])

AC_DEFUN([gl_C_STRTOLD],
[
  dnl Prerequisites of lib/c-strtold.c.
  AC_REQUIRE([gl_C_STRTOD])
  AC_REQUIRE([gl_C99_STRTOLD])
  :
])
