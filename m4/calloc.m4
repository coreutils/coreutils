# calloc.m4 serial 3

# Copyright (C) 2004 Free Software Foundation, Inc.

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

# Written by Jim Meyering.

# Determine whether calloc (N, S) returns non-NULL when N*S is zero,
# and returns NULL when N*S overflows.
# If so, define HAVE_CALLOC.  Otherwise, define calloc to rpl_calloc
# and arrange to use a calloc wrapper function that does work in that case.

# _AC_FUNC_CALLOC_IF(IF-WORKS, IF-NOT)
# -------------------------------------
# If `calloc (0, 0)' is properly handled, run IF-WORKS, otherwise, IF-NOT.
AC_DEFUN([_AC_FUNC_CALLOC_IF],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_REQUIRE([AC_TYPE_SIZE_T])dnl
AC_CHECK_HEADERS(stdlib.h)
AC_CACHE_CHECK([for GNU libc compatible calloc], ac_cv_func_calloc_0_nonnull,
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
		  [exit (!calloc (0, 0) || calloc ((size_t) -1 / 8 + 1, 8));])],
	       [ac_cv_func_calloc_0_nonnull=yes],
	       [ac_cv_func_calloc_0_nonnull=no],
	       [ac_cv_func_calloc_0_nonnull=no])])
AS_IF([test $ac_cv_func_calloc_0_nonnull = yes], [$1], [$2])
])# AC_FUNC_CALLOC


# AC_FUNC_CALLOC
# ---------------
# Report whether `calloc (0, 0)' is properly handled, and replace calloc if
# needed.
AC_DEFUN([AC_FUNC_CALLOC],
[_AC_FUNC_CALLOC_IF(
  [AC_DEFINE([HAVE_CALLOC], 1,
	     [Define to 1 if your system has a GNU libc compatible `calloc'
	      function, and to 0 otherwise.])],
  [AC_DEFINE([HAVE_CALLOC], 0)
   AC_LIBOBJ([calloc])
   AC_DEFINE([calloc], [rpl_calloc],
      [Define to rpl_calloc if the replacement function should be used.])])
])# AC_FUNC_CALLOC
