#serial 3

# Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_REQUIRE([AC_HEADER_STDC])
    AC_CHECK_HEADERS(string.h sys/param.h stddef.h)
    AC_CHECK_FUNCS(resolvepath canonicalize_file_name)
    AC_REQUIRE([AC_HEADER_STAT])
  ])
