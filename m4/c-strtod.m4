# c-strtod.m4 serial 3

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

# Written by Paul Eggert.

AC_DEFUN([gl_C_STRTOD],
[
  dnl Prerequisites of lib/c-strtod.c.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  :
])

AC_DEFUN([gl_C_STRTOLD],
[
  dnl Prerequisites of lib/c-strtold.c.
  AC_REQUIRE([gl_C_STRTOD])
  AC_CHECK_DECLS_ONCE([strtold])
])
