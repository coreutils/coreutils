#serial 8

# Copyright (C) 1996, 1999, 2001, 2002, 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering and Paul Eggert.
AC_DEFUN([gl_HEADER_TIOCGWINSZ_IN_TERMIOS_H],
[AC_REQUIRE([AC_SYS_POSIX_TERMIOS])
 AC_CACHE_CHECK([whether use of TIOCGWINSZ requires termios.h],
	        jm_cv_sys_tiocgwinsz_needs_termios_h,
  [jm_cv_sys_tiocgwinsz_needs_termios_h=no

   if test $ac_cv_sys_posix_termios = yes; then
     AC_EGREP_CPP([yes],
     [#include <sys/types.h>
#      include <termios.h>
#      ifdef TIOCGWINSZ
         yes
#      endif
     ], jm_cv_sys_tiocgwinsz_needs_termios_h=yes)
   fi
  ])
])

AC_DEFUN([gl_WINSIZE_IN_PTEM],
  [AC_REQUIRE([AC_SYS_POSIX_TERMIOS])
   AC_CACHE_CHECK([whether use of struct winsize requires sys/ptem.h],
     jm_cv_sys_struct_winsize_needs_sys_ptem_h,
     [jm_cv_sys_struct_winsize_needs_sys_ptem_h=yes
      if test $ac_cv_sys_posix_termios = yes; then
	AC_TRY_COMPILE([#include <termios.h>]
	  [struct winsize x;],
          [jm_cv_sys_struct_winsize_needs_sys_ptem_h=no])
      fi
      if test $jm_cv_sys_struct_winsize_needs_sys_ptem_h = yes; then
	AC_TRY_COMPILE([#include <sys/ptem.h>],
	  [struct winsize x;],
	  [], [jm_cv_sys_struct_winsize_needs_sys_ptem_h=no])
      fi])
   if test $jm_cv_sys_struct_winsize_needs_sys_ptem_h = yes; then
     AC_DEFINE([WINSIZE_IN_PTEM], 1,
       [Define if sys/ptem.h is required for struct winsize.])
   fi])
