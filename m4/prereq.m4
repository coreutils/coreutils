#serial 73

dnl We use gl_ for non Autoconf macros.
m4_pattern_forbid([^gl_[ABCDEFGHIJKLMNOPQRSTUVXYZ]])dnl

# These are the prerequisite macros for files in the lib/
# directory of the coreutils package.


# Copyright (C) 1998, 2000, 2001, 2003, 2004, 2005, 2006 Free Software
# Foundation, Inc.

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
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# Written by Jim Meyering.


AC_DEFUN([gl_PREREQ],
[
  # gnulib settings.
  AC_REQUIRE([gl_FUNC_NONREENTRANT_EUIDACCESS])

  # We don't use c-stack.c.
  # AC_REQUIRE([gl_C_STACK])

  # Invoke macros of modules that may migrate into gnulib.
  # There's no need to list gnulib modules here, since gnulib-tool
  # handles that; see ../bootstrap.conf.
  AC_REQUIRE([gl_EUIDACCESS_STAT])
  AC_REQUIRE([gl_FD_REOPEN])
  AC_REQUIRE([gl_FUNC_XFTS])
  AC_REQUIRE([gl_MEMXFRM])
  AC_REQUIRE([gl_RANDINT])
  AC_REQUIRE([gl_RANDPERM])
  AC_REQUIRE([gl_RANDREAD])
  AC_REQUIRE([gl_ROOT_DEV_INO])
  AC_REQUIRE([gl_SHA256])
  AC_REQUIRE([gl_SHA512])
  AC_REQUIRE([gl_STRINTCMP])
  AC_REQUIRE([gl_STRNUMCMP])
])
