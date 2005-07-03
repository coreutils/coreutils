#serial 14

# Copyright (C) 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005 Free Software
# Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.

AC_DEFUN([gl_FUNC_LSTAT],
[
  AC_LIBSOURCES([lstat.c, lstat.h])

  AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
  dnl Note: AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK does AC_LIBOBJ(lstat).
  :
])
