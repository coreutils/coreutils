# uintptr_t.m4 serial 3

# Copyright (C) 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

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
