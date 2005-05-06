#serial 2

# Copyright (C) 2005 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STAT_MACROS],
[
  AC_LIBSOURCES([stat-macros.h])

  AC_REQUIRE([AC_HEADER_STAT])
])
