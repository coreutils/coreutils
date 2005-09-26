# Detect some bugs in glibc's implementation of utimes.

dnl Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# See if we need to work around bugs in glibc's implementation of
# utimes from 2003-07-12 to 2003-09-17.
# First, there was a bug that would make utimes set mtime
# and atime to zero (1970-01-01) unconditionally.
# Then, there was code to round rather than truncate.
# Then, there was an implementation (sparc64, Linux-2.4.28, glibc-2.3.3)
# that didn't honor the NULL-means-set-to-current-time semantics.
# Finally, there was also a version of utimes that failed on read-only
# files, while utime worked fine (linux-2.2.20, glibc-2.2.5).
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
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
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
  time_t now;
  int fd;

  int ok = ((f = fopen (file, "w"))
	    && fclose (f) == 0
	    && utimes (file, timeval) == 0
	    && lstat (file, &sbuf) == 0
	    && sbuf.st_atime == timeval[0].tv_sec
	    && sbuf.st_mtime == timeval[1].tv_sec);
  unlink (file);
  if (!ok)
    exit (1);

  ok =
    ((f = fopen (file, "w"))
     && fclose (f) == 0
     && time (&now) != (time_t)-1
     && utimes (file, NULL) == 0
     && lstat (file, &sbuf) == 0
     && now - sbuf.st_atime <= 2
     && now - sbuf.st_mtime <= 2);
  unlink (file);
  if (!ok)
    exit (1);

  ok = (0 <= (fd = open (file, O_WRONLY|O_CREAT, 0444))
	      && close (fd) == 0
	      && utimes (file, NULL) == 0);
  unlink (file);

  exit (!ok);
}
  ]])],
       [gl_cv_func_working_utimes=yes],
       [gl_cv_func_working_utimes=no],
       [gl_cv_func_working_utimes=no])])

  if test $gl_cv_func_working_utimes = yes; then
    AC_DEFINE([HAVE_WORKING_UTIMES], 1, [Define if utimes works properly. ])
  fi
])
