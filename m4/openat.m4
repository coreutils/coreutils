#serial 2
# See if we need to use our replacement for Solaris' openat function.

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

AC_DEFUN([gl_FUNC_OPENAT],
[
  AC_LIBSOURCES([openat.c, openat.h])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_REPLACE_FUNCS(openat)
  case $ac_cv_func_openat in
  yes) ;;
  *)
    AC_DEFINE([__OPENAT_PREFIX], [[rpl_]],
      [Define to rpl_ if the openat replacement function should be used.])
    gl_PREREQ_OPENAT;;
  esac
])

AC_DEFUN([gl_PREREQ_OPENAT],
[
  AC_REQUIRE([gl_SAVE_CWD])
])
