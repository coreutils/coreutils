#serial 7

# Copyright (C) 1998, 1999, 2001, 2003, 2004, 2006 Free Software
# Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl Shamelessly cloned from acspecific.m4's AC_FUNC_UTIME_NULL,
dnl then do case-insensitive s/utime/utimes/.

AC_DEFUN([gl_FUNC_UTIMES_NULL],
[AC_CACHE_CHECK(whether utimes accepts a null argument, ac_cv_func_utimes_null,
[rm -f conftest.data; > conftest.data
AC_TRY_RUN([
/* In case stat has been defined to rpl_stat, undef it here.  */
#undef stat
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
int
main () {
struct stat s, t;
return ! (stat ("conftest.data", &s) == 0
	  && utimes ("conftest.data", 0) == 0
	  && stat ("conftest.data", &t) == 0
	  && t.st_mtime >= s.st_mtime
	  && t.st_mtime - s.st_mtime < 120));
}],
  ac_cv_func_utimes_null=yes,
  ac_cv_func_utimes_null=no,
  ac_cv_func_utimes_null=no)
rm -f core core.* *.core])

    if test $ac_cv_func_utimes_null = yes; then
      AC_DEFINE(HAVE_UTIMES_NULL, 1,
		[Define if utimes accepts a null argument])
    fi
  ]
)
