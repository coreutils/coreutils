# See if we need to work around bugs in glibc's implementation of
# utimes from around July/August 2003.
# First, there was a bug that would make utimes set mtime
# and atime to zero (1970-01-01) unconditionally.
# Then, there is/was code to round rather than truncate.
#
# From Jim Meyering, with suggestions from Paul Eggert.

AC_DEFUN([gl_FUNC_UTIMES],
[
  AC_CACHE_CHECK([determine whether the utimes function works],
		 gl_cv_func_working_utimes,
  [
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <utime.h>

int
main ()
{
  static struct timeval timeval[2] = {{9, 10}, {999999, 999999}};
  struct stat sbuf;
  char const *file = "conftest.utimes";
  FILE *f;

  exit ( ! ((f = fopen (file, "w"))
	    && fclose (f) == 0
	    && utimes (file, timeval) == 0
	    && lstat (file, &sbuf) == 0
	    && sbuf.st_atime == timeval[0].tv_sec
	    && sbuf.st_mtime == timeval[1].tv_sec) );
}
  ]])],
       [gl_cv_func_working_utimes=yes],
       [gl_cv_func_working_utimes=no],
       [gl_cv_func_working_utimes=no])])

  if test $gl_cv_func_working_utimes = yes; then
    AC_DEFINE([HAVE_WORKING_UTIMES], 1, [Define if utimes works properly. ])
  fi
])
