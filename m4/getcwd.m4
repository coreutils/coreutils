# getcwd.m4 - check whether getcwd (NULL, 0) allocates memory for result

# Copyright 2001 Free Software Foundation, Inc.

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
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

# Written by Paul Eggert.

AC_DEFUN([AC_FUNC_GETCWD_NULL],
  [AC_CHECK_HEADERS(stdlib.h unistd.h)
   AC_CACHE_CHECK([whether getcwd (NULL, 0) allocates memory for result],
     [ac_cv_func_getcwd_null],
     [AC_TRY_RUN(
        [
#	 ifdef HAVE_STDLIB_H
#	  include <stdlib.h>
#	 endif
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
	[ac_cv_func_getcwd_null=yes],
	[ac_cv_func_getcwd_null=no],
	[ac_cv_func_getcwd_null=no])])
   if test $ac_cv_func_getcwd_null = yes; then
     AC_DEFINE(HAVE_GETCWD_NULL, 1,
	       [Define if getcwd (NULL, 0) allocates memory for result.])
   fi])
