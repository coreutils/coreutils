#serial 1

# Copyright (C) 2005 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([gl_FUNC_XANSTRFTIME],
[
 AC_LIBSOURCES([xanstrftime.c, xanstrftime.h])
 AC_LIBOBJ([xanstrftime])
 # depends on
 # xalloc
 # xalloc-die
 # stdbool
 # strftime
 # memcpy
 # free
])
