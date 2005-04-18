# clock_time.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Check for clock_gettime and clock_settime, and sets LIB_CLOCK_GETTIME.
AC_DEFUN([gl_CLOCK_TIME],
[
  # dnl Persuade glibc <time.h> to declare these functions.
  AC_REQUIRE([AC_GNU_SOURCE])

  # Solaris 2.5.1 needs -lposix4 to get the clock_gettime function.
  # Solaris 7 prefers the library name -lrt to the obsolescent name -lposix4.

  # Save and restore LIBS so e.g., -lrt, isn't added to it.  Otherwise, *all*
  # programs in the package would end up linked with that potentially-shared
  # library, inducing unnecessary run-time overhead.
  gl_saved_libs=$LIBS
    AC_SEARCH_LIBS(clock_gettime, [rt posix4],
                   [test "$ac_cv_search_clock_gettime" = "none required" ||
                    LIB_CLOCK_GETTIME=$ac_cv_search_clock_gettime])
    AC_SUBST(LIB_CLOCK_GETTIME)
    AC_CHECK_FUNCS(clock_gettime clock_settime)
  LIBS=$gl_saved_libs
])
