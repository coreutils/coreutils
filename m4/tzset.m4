#serial 2

# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# See if we have a working tzset function.
# If so, arrange to compile the wrapper function.
# For at least Solaris 2.5.1 and 2.6, this is necessary
# because tzset can clobber the contents of the buffer
# used by localtime.

# Written by Paul Eggert and Jim Meyering.

AC_DEFUN([gl_FUNC_TZSET_CLOBBER],
[
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CACHE_CHECK([whether tzset clobbers localtime buffer],
                 gl_cv_func_tzset_clobber,
  [
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <stdlib.h>

int
main ()
{
  time_t t1 = 853958121;
  struct tm *p, s;
  putenv ("TZ=GMT0");
  p = localtime (&t1);
  s = *p;
  putenv ("TZ=EST+3EDT+2,M10.1.0/00:00:00,M2.3.0/00:00:00");
  tzset ();
  exit (p->tm_year != s.tm_year
        || p->tm_mon != s.tm_mon
        || p->tm_mday != s.tm_mday
        || p->tm_hour != s.tm_hour
        || p->tm_min != s.tm_min
        || p->tm_sec != s.tm_sec);
}
  ]])],
       [gl_cv_func_tzset_clobber=no],
       [gl_cv_func_tzset_clobber=yes],
       [gl_cv_func_tzset_clobber=yes])])

  AC_DEFINE(HAVE_RUN_TZSET_TEST, 1,
    [Define to 1 if you have run the test for working tzset.])

  if test $gl_cv_func_tzset_clobber = yes; then
    gl_GETTIMEOFDAY_REPLACE_LOCALTIME

    AC_DEFINE(tzset, rpl_tzset,
      [Define to rpl_tzset if the wrapper function should be used.])
    AC_DEFINE(TZSET_CLOBBERS_LOCALTIME_BUFFER, 1,
      [Define if tzset clobbers localtime's static buffer.])
  fi
])
