#serial 17

dnl From Jim Meyering.
dnl Check for the nanosleep function.
dnl If not found, use the supplied replacement.
dnl

# Copyright (C) 1999, 2000, 2001, 2003, 2004, 2005, 2006 Free Software
# Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_NANOSLEEP],
[
 AC_LIBSOURCES([nanosleep.c])

 dnl Persuade glibc and Solaris <time.h> to declare nanosleep.
 AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

 AC_REQUIRE([AC_HEADER_TIME])
 AC_CHECK_HEADERS_ONCE(sys/time.h)

 nanosleep_save_libs=$LIBS

 # Solaris 2.5.1 needs -lposix4 to get the nanosleep function.
 # Solaris 7 prefers the library name -lrt to the obsolescent name -lposix4.
 AC_SEARCH_LIBS([nanosleep], [rt posix4],
                [test "$ac_cv_search_nanosleep" = "none required" ||
	         LIB_NANOSLEEP=$ac_cv_search_nanosleep])
 AC_SUBST([LIB_NANOSLEEP])

 AC_CACHE_CHECK([for nanosleep],
  [gl_cv_func_nanosleep],
  [
   AC_LINK_IFELSE([AC_LANG_SOURCE([[
#   if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#    include <time.h>
#   else
#    if HAVE_SYS_TIME_H
#     include <sys/time.h>
#    else
#     include <time.h>
#    endif
#   endif

    int
    main ()
    {
      struct timespec ts_sleep, ts_remaining;
      ts_sleep.tv_sec = 0;
      ts_sleep.tv_nsec = 1;
      return nanosleep (&ts_sleep, &ts_remaining) < 0;
    }
      ]])],
     [gl_cv_func_nanosleep=yes],
     [gl_cv_func_nanosleep=no])
  ])
  if test $gl_cv_func_nanosleep = no; then
    AC_LIBOBJ(nanosleep)
    AC_DEFINE(nanosleep, rpl_nanosleep,
      [Define to rpl_nanosleep if the replacement function should be used.])
    gl_PREREQ_NANOSLEEP
  fi

 LIBS=$nanosleep_save_libs
])

# Prerequisites of lib/nanosleep.c.
AC_DEFUN([gl_PREREQ_NANOSLEEP],
[
  AC_CHECK_FUNCS_ONCE(siginterrupt)
  AC_CHECK_HEADERS_ONCE(sys/select.h)
])
