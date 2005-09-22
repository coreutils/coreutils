# calloc.m4 serial 5

# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

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
