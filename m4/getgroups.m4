#serial 10

dnl From Jim Meyering.
dnl A wrapper around AC_FUNC_GETGROUPS.

# Copyright (C) 1996, 1997, 1999, 2000, 2001, 2002, 2003, 2004 Free
# Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_GETGROUPS],
[
  AC_REQUIRE([AC_FUNC_GETGROUPS])
  if test $ac_cv_func_getgroups_works = no; then
    AC_LIBOBJ(getgroups)
    AC_DEFINE(getgroups, rpl_getgroups,
      [Define as rpl_getgroups if getgroups doesn't work right.])
    gl_PREREQ_GETGROUPS
  fi
  test -n "$GETGROUPS_LIB" && LIBS="$GETGROUPS_LIB $LIBS"
])

# Prerequisites of lib/getgroups.c.
AC_DEFUN([gl_PREREQ_GETGROUPS],
[
  AC_REQUIRE([AC_TYPE_GETGROUPS])
])
