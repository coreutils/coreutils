#serial 23

# Copyright (C) 1996, 1997, 1999, 2000, 2001, 2002, 2003, 2004
# Free Software Foundation, Inc.

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

# Written by Jim Meyering and Paul Eggert.

AC_DEFUN([gl_FUNC_GNU_STRFTIME],
[AC_REQUIRE([gl_FUNC_STRFTIME])dnl
 AC_REQUIRE([AC_C_CONST])dnl
])

# These are the prerequisite macros for GNU's strftime.c replacement.
AC_DEFUN([gl_FUNC_STRFTIME],
[
 # strftime.c uses the underyling system strftime if it exists.
 AC_REQUIRE([AC_FUNC_STRFTIME])

 # This defines (or not) HAVE_TZNAME and HAVE_TM_ZONE.
 AC_REQUIRE([AC_STRUCT_TIMEZONE])

 AC_REQUIRE([AC_HEADER_TIME])
 AC_REQUIRE([AC_TYPE_MBSTATE_T])
 AC_REQUIRE([gl_TM_GMTOFF])
 AC_REQUIRE([gl_FUNC_TZSET_CLOBBER])

 AC_CHECK_FUNCS_ONCE(mblen mbrlen mempcpy tzset)
 AC_CHECK_HEADERS_ONCE(sys/time.h wchar.h)

 AC_DEFINE([my_strftime], [nstrftime],
   [Define to the name of the strftime replacement function.])
])
