#serial 25

# Copyright (C) 1996, 1997, 1999, 2000, 2001, 2002, 2003, 2004, 2005
# Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering and Paul Eggert.

AC_DEFUN([gl_FUNC_GNU_STRFTIME],
[AC_REQUIRE([gl_FUNC_STRFTIME])dnl
 AC_REQUIRE([AC_C_CONST])dnl
])

# These are the prerequisite macros for GNU's strftime.c replacement.
AC_DEFUN([gl_FUNC_STRFTIME],
[
 AC_LIBSOURCES([strftime.c, strftime.h])
 AC_LIBOBJ([strftime])

 # strftime.c uses the underyling system strftime if it exists.
 AC_REQUIRE([AC_FUNC_STRFTIME])

 # This defines (or not) HAVE_TZNAME and HAVE_TM_ZONE.
 AC_REQUIRE([AC_STRUCT_TIMEZONE])

 AC_REQUIRE([AC_HEADER_TIME])
 AC_REQUIRE([AC_TYPE_MBSTATE_T])
 AC_REQUIRE([gl_TM_GMTOFF])

 AC_CHECK_FUNCS_ONCE(mblen mbrlen mempcpy tzset)
 AC_CHECK_HEADERS_ONCE(sys/time.h wchar.h)

 AC_DEFINE([my_strftime], [nstrftime],
   [Define to the name of the strftime replacement function.])
])
