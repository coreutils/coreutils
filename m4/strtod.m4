## Copyright (C) 1996 Free Software Foundation, Inc.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.

## From Jim Meyering.

## serial 1

## @defmac AC_FUNC_STRTOD
## @maindex FUNC_STRTOD
## @ovindex LIBOBJS
## If the @code{strtod} function is not available, or does not work
## correctly (like the one on SunOS 5.4), add @samp{strtod.o} to output
## variable @code{LIBOBJS}.
## @end defmac

AC_DEFUN(AM_FUNC_STRTOD,
[AC_CACHE_CHECK(for working strtod, am_cv_func_strtod,
[AC_TRY_RUN([
double strtod ();
int
main()
{
  {
    /* Some versions of Linux strtod mis-parse strings with leading '+'.  */
    char *string = " +69";
    char *term;
    double value;
    value = strtod (string, &term);
    if (value != 69 || term != (string + 4))
      exit (1);
  }

  {
    /* Under Solaris 2.4, strtod returns the wrong value for the
       terminating character under some conditions.  */
    char *string = "NaN";
    char *term;
    strtod (string, &term);
    if (term != string && *(term - 1) == 0)
      exit (1);
  }
  exit (0);
}
], am_cv_func_strtod=yes, am_cv_func_strtod=no, am_cv_func_strtod=no)])
test $am_cv_func_strtod = no && LIBOBJS="$LIBOBJS strtod.o"
AC_SUBST(LIBOBJS)dnl
am_cv_func_strtod_needs_libm=no
if test $am_cv_func_strtod = no; then
  AC_CHECK_FUNCS(pow)
  if test $ac_cv_func_pow = no; then
    AC_CHECK_LIB(m, pow, [am_cv_func_strtod_needs_libm=yes],
		 [AC_MSG_WARN(can't find library containing definition of pow)])
  fi
fi
])
