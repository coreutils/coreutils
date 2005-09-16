# Checks for stat-related time functions.

# Copyright (C) 1998, 1999, 2001, 2003, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

# st_atim.tv_nsec - Linux, Solaris
# st_atimespec.tv_nsec - FreeBSD, if ! defined _POSIX_SOURCE
# st_atimensec - FreeBSD, if defined _POSIX_SOURCE
# st_atim.st__tim.tv_nsec - UnixWare (at least 2.1.2 through 7.1)
# st_spare1 - Cygwin?

AC_DEFUN([gl_STAT_TIME],
[
  AC_LIBSOURCES([stat-time.h])

  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_MEMBERS([struct stat.st_atim.tv_nsec], [],
    [AC_CHECK_MEMBERS([struct stat.st_atimespec.tv_nsec], [],
       [AC_CHECK_MEMBERS([struct stat.st_atimensec], [],
	  [AC_CHECK_MEMBERS([struct stat.st_atim.st__tim.tv_nsec], [],
	     [AC_CHECK_MEMBERS([struct stat.st_spare1], [],
		[],
		[#include <sys/types.h>
		 #include <sys/stat.h>])],
	     [#include <sys/types.h>
	      #include <sys/stat.h>])],
	  [#include <sys/types.h>
	   #include <sys/stat.h>])],
       [#include <sys/types.h>
	#include <sys/stat.h>])],
    [#include <sys/types.h>
     #include <sys/stat.h>])
])
