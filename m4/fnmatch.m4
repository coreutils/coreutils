# Check for fnmatch.

# This is a modified version of autoconf's AC_FUNC_FNMATCH.
# This file should be removed after Autoconf 2.54 is required.

# Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# _AC_FUNC_FNMATCH_IF(STANDARD = GNU | POSIX, CACHE_VAR, IF-TRUE, IF-FALSE)
# -------------------------------------------------------------------------
# If a STANDARD compliant fnmatch is found, run IF-TRUE, otherwise
# IF-FALSE.  Use CACHE_VAR.
AC_DEFUN([_AC_FUNC_FNMATCH_IF],
[AC_CACHE_CHECK(
   [for working $1 fnmatch],
   [$2],
  [# Some versions of Solaris, SCO, and the GNU C Library
   # have a broken or incompatible fnmatch.
   # So we run a test program.  If we are cross-compiling, take no chance.
   # Thanks to John Oleynick, Franc,ois Pinard, and Paul Eggert for this test.
   AC_RUN_IFELSE(
      [AC_LANG_PROGRAM(
	 [#include <fnmatch.h>
#	   define y(a, b, c) (fnmatch (a, b, c) == 0)
#	   define n(a, b, c) (fnmatch (a, b, c) == FNM_NOMATCH)
         ],
	 [exit
	   (!(y ("a*", "abc", 0)
	      && n ("d*/*1", "d/s/1", FNM_PATHNAME)
	      && y ("a\\\\bc", "abc", 0)
	      && n ("a\\\\bc", "abc", FNM_NOESCAPE)
	      && y ("*x", ".x", 0)
	      && n ("*x", ".x", FNM_PERIOD)
	      && m4_if([$1], [GNU],
		   [y ("xxXX", "xXxX", FNM_CASEFOLD)
		    && y ("a++(x|yy)b", "a+xyyyyxb", FNM_EXTMATCH)
		    && n ("d*/*1", "d/s/1", FNM_FILE_NAME)
		    && y ("*", "x", FNM_FILE_NAME | FNM_LEADING_DIR)
		    && y ("x*", "x/y/z", FNM_FILE_NAME | FNM_LEADING_DIR)
		    && y ("*c*", "c/x", FNM_FILE_NAME | FNM_LEADING_DIR)],
		   1)));])],
      [$2=yes],
      [$2=no],
      [$2=cross])])
AS_IF([test $$2 = yes], [$3], [$4])
])# _AC_FUNC_FNMATCH_IF


# _AC_LIBOBJ_FNMATCH
# ------------------
# Prepare the replacement of fnmatch.
AC_DEFUN([_AC_LIBOBJ_FNMATCH],
[AC_REQUIRE([AC_C_CONST])dnl
AC_REQUIRE([AC_FUNC_ALLOCA])dnl
AC_REQUIRE([AC_TYPE_MBSTATE_T])dnl
AC_CHECK_DECLS([getenv])
AC_CHECK_FUNCS([btowc mbsrtowcs mempcpy wmempcpy])
AC_CHECK_HEADERS([wchar.h wctype.h])
AC_LIBOBJ([fnmatch])
AC_CONFIG_LINKS([lib/fnmatch.h:lib/fnmatch_.h])
AC_DEFINE(fnmatch, rpl_fnmatch,
          [Define to rpl_fnmatch if the replacement function should be used.])
])# _AC_LIBOBJ_FNMATCH


# AC_FUNC_FNMATCH_GNU
# -------------------
AC_DEFUN([AC_FUNC_FNMATCH_GNU],
[AC_REQUIRE([AC_GNU_SOURCE])
_AC_FUNC_FNMATCH_IF([GNU], [ac_cv_func_fnmatch_gnu],
                    [rm -f lib/fnmatch.h],
                    [_AC_LIBOBJ_FNMATCH])
])# AC_FUNC_FNMATCH_GNU
