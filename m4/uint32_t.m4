# uint32_t.m4 serial 3

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

AC_DEFUN([gl_AC_TYPE_UINT32_T],
[
  AC_CACHE_CHECK([for uint32_t], gl_cv_c_uint32_t,
    [gl_cv_c_uint32_t=no
     for ac_type in "uint32_t" "unsigned int" \
	 "unsigned long int" "unsigned short int"; do
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [AC_INCLUDES_DEFAULT],
	    [[($ac_type) -1 == 4294967295U]])],
	 [gl_cv_c_uint32_t=$ac_type])
       test "$gl_cv_c_uint32_t" != no && break
     done])
  case "$gl_cv_c_uint32_t" in
  no|uint32_t) ;;
  *)
    AC_DEFINE(_UINT32_T, 1,
      [Define for Solaris 2.5.1 so uint32_t typedef from <sys/synch.h>,
       <pthread.h>, or <semaphore.h> is not used. If the typedef was
       allowed, the #define below would cause a syntax error.])
    AC_DEFINE_UNQUOTED(uint32_t, $gl_cv_c_uint32_t,
      [Define to the type of a unsigned integer type of width exactly 32 bits
       if such a type exists and the standard includes do not define it.])
    ;;
  esac

  AC_CACHE_CHECK([for UINT32_MAX], gl_cv_c_uint32_max,
    [AC_COMPILE_IFELSE(
       [AC_LANG_BOOL_COMPILE_TRY(
	  [AC_INCLUDES_DEFAULT],
	  [[UINT32_MAX == 4294967295U]])],
       [gl_cv_c_uint32_max=yes],
       [gl_cv_c_uint32_max=no])])
  case $gl_cv_c_uint32_max,$gl_cv_c_uint32_t in
  yes,*) ;;
  *,no) ;;
  *)
    AC_DEFINE(UINT32_MAX, 4294967295U,
      [Define to its maximum value if an unsigned integer type of width
       exactly 32 bits exists and the standard includes do not define
       UINT32_MAX.])
    ;;
  esac
])
