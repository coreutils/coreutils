# Check for fnmatch.

# This is a modified version of autoconf's AC_FUNC_FNMATCH.
# This file should be simplified after Autoconf 2.57 is required.

# Copyright (C) 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AC_FUNC_FNMATCH_IF(STANDARD = GNU | POSIX, CACHE_VAR, IF-TRUE, IF-FALSE)
# -------------------------------------------------------------------------
# If a STANDARD compliant fnmatch is found, run IF-TRUE, otherwise
# IF-FALSE.  Use CACHE_VAR.
AC_DEFUN([_AC_FUNC_FNMATCH_IF],
[AC_CACHE_CHECK(
   [for working $1 fnmatch],
   [$2],
  [dnl Some versions of Solaris, SCO, and the GNU C Library
   dnl have a broken or incompatible fnmatch.
   dnl So we run a test program.  If we are cross-compiling, take no chance.
   dnl Thanks to John Oleynick, François Pinard, and Paul Eggert for this test.
   AC_RUN_IFELSE(
      [AC_LANG_PROGRAM(
	 [
#	   include <stdlib.h>
#	   include <fnmatch.h>
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
AC_CHECK_FUNCS([btowc mbsrtowcs mempcpy wmemchr wmemcpy wmempcpy])
AC_CHECK_HEADERS([wchar.h wctype.h])
AC_LIBOBJ([fnmatch])
FNMATCH_H=fnmatch.h
])# _AC_LIBOBJ_FNMATCH


AC_DEFUN([gl_FUNC_FNMATCH_POSIX],
[
  FNMATCH_H=
  _AC_FUNC_FNMATCH_IF([POSIX], [ac_cv_func_fnmatch_posix],
                      [rm -f lib/fnmatch.h],
                      [_AC_LIBOBJ_FNMATCH])
  if test $ac_cv_func_fnmatch_posix != yes; then
    dnl We must choose a different name for our function, since on ELF systems
    dnl a broken fnmatch() in libc.so would override our fnmatch() if it is
    dnl compiled into a shared library.
    AC_DEFINE([fnmatch], [posix_fnmatch],
      [Define to a replacement function name for fnmatch().])
  fi
  AC_SUBST([FNMATCH_H])
])


AC_DEFUN([gl_FUNC_FNMATCH_GNU],
[
  dnl Persuade glibc <fnmatch.h> to declare FNM_CASEFOLD etc.
  AC_REQUIRE([AC_GNU_SOURCE])

  FNMATCH_H=
  _AC_FUNC_FNMATCH_IF([GNU], [ac_cv_func_fnmatch_gnu],
                      [rm -f lib/fnmatch.h],
                      [_AC_LIBOBJ_FNMATCH])
  if test $ac_cv_func_fnmatch_gnu != yes; then
    dnl We must choose a different name for our function, since on ELF systems
    dnl a broken fnmatch() in libc.so would override our fnmatch() if it is
    dnl compiled into a shared library.
    AC_DEFINE([fnmatch], [gnu_fnmatch],
      [Define to a replacement function name for fnmatch().])
  fi
  AC_SUBST([FNMATCH_H])
])
