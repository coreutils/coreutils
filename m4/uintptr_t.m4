# uintptr_t.m4 serial 2

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

AC_DEFUN([gl_AC_TYPE_UINTPTR_T],
[
  AC_CACHE_CHECK([for uintptr_t], gl_cv_c_uintptr_t,
    [gl_cv_c_uintptr_t=no
     for ac_type in "uintptr_t" "unsigned int" \
	 "unsigned long int" "unsigned long long int"; do
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [AC_INCLUDES_DEFAULT],
	    [[sizeof (void *) <= sizeof (uintptr_t)]])],
	 [gl_cv_c_uintptr_t=$ac_type])
       test $gl_cv_c_uintptr_t != no && break
     done])
  case $gl_cv_c_uintptr_t in
  no|uintptr_t) ;;
  *)
    AC_DEFINE_UNQUOTED(uintptr_t, $gl_cv_c_uintptr_t,
      [Define to the type of a unsigned integer type wide enough to
       hold a pointer, if such a type exists.])
    ;;
  esac

  dnl Check whether UINTPTR_MAX is defined, not whether it has the
  dnl right value.  Alas, Solaris 8 defines it to empty!
  dnl Applications should use (uintptr_t) -1 rather than UINTPTR_MAX.
  AC_CACHE_CHECK([for UINTPTR_MAX], gl_cv_c_uintptr_max,
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [AC_INCLUDES_DEFAULT],
	  [[#ifndef UINTPTR_MAX
	     error: UINTPTR_MAX is not defined.
	    #endif]])],
       [gl_cv_c_uintptr_max=yes],
       [gl_cv_c_uintptr_max=no])])
  case $gl_cv_c_uintptr_max,$gl_cv_c_uintptr_t in
  yes,*) ;;
  *,no) ;;
  *)
    AC_DEFINE(UINTPTR_MAX, ((uintptr_t) -1),
      [Define to its maximum value if an unsigned integer type wide enough
       to hold a pointer exists and the standard includes do not define
       UINTPTR_MAX.])
    ;;
  esac
])
