# Check for stdbool.h that conforms to C99.

dnl Copyright (C) 2002-2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prepare for substituting <stdbool.h> if it is not supported.

AC_DEFUN([AM_STDBOOL_H],
[
  AC_REQUIRE([AC_HEADER_STDBOOL])

  # Define two additional variables used in the Makefile substitution.

  if test "$ac_cv_header_stdbool_h" = yes; then
    STDBOOL_H=''
  else
    STDBOOL_H='stdbool.h'
  fi
  AC_SUBST([STDBOOL_H])

  if test "$ac_cv_type__Bool" = yes; then
    HAVE__BOOL=1
  else
    HAVE__BOOL=0
  fi
  AC_SUBST([HAVE__BOOL])
])

# This macro is only needed in autoconf <= 2.59.  Newer versions of autoconf
# have this macro built-in.

AC_DEFUN([AC_HEADER_STDBOOL],
  [AC_CACHE_CHECK([for stdbool.h that conforms to C99],
     [ac_cv_header_stdbool_h],
     [AC_TRY_COMPILE(
	[
	  #include <stdbool.h>
	  #ifndef bool
	   "error: bool is not defined"
	  #endif
	  #ifndef false
	   "error: false is not defined"
	  #endif
	  #if false
	   "error: false is not 0"
	  #endif
	  #ifndef true
	   "error: true is not defined"
	  #endif
	  #if true != 1
	   "error: true is not 1"
	  #endif
	  #ifndef __bool_true_false_are_defined
	   "error: __bool_true_false_are_defined is not defined"
	  #endif

	  struct s { _Bool s: 1; _Bool t; } s;

	  char a[true == 1 ? 1 : -1];
	  char b[false == 0 ? 1 : -1];
	  char c[__bool_true_false_are_defined == 1 ? 1 : -1];
	  char d[(bool) -0.5 == true ? 1 : -1];
	  bool e = &s;
	  char f[(_Bool) -0.0 == false ? 1 : -1];
	  char g[true];
	  char h[sizeof (_Bool)];
	  char i[sizeof s.t];
	  enum { j = false, k = true, l = false * true, m = true * 256 };
	  _Bool n[m];
	  char o[sizeof n == m * sizeof n[0] ? 1 : -1];
	  char p[-1 - (_Bool) 0 < 0 && -1 - (bool) 0 < 0 ? 1 : -1];
	  #if defined __xlc__ || __GNUC__
	   /* Catch a bug in IBM AIX xlc compiler version 6.0.0.0
	      reported by James Lemley on 2005-10-05; see
	      <http://lists.gnu.org/archive/html/bug-coreutils/2005-10/msg00086.html>.
	      This test is not quite right, since xlc is allowed to
	      reject this program, as the initializer for xlcbug is
	      not one of the forms that C requires support for.
	      However, doing the test right would require a run-time
	      test, and that would make crosss-compilation harder.
	      Let us hope that IBM fixes the xlc bug, and also adds
	      support for this kind of constant expression.  In the
	      meantime, this test will reject xlc, which is OK, since
	      our stdbool.h substitute should suffice.  */
	   char digs[] = "0123456789";
	   int xlcbug = 1 / (&(digs + 5)[-2 + (bool) 1] == &digs[4] ? 1 : -1);
	  #endif
	],
	[
	  return (!a + !b + !c + !d + !e + !f + !g + !h + !i + !j + !k + !l
		  + !m + !n + !o + !p);
	],
	[ac_cv_header_stdbool_h=yes],
	[ac_cv_header_stdbool_h=no])])
   AC_CHECK_TYPES([_Bool])
   if test $ac_cv_header_stdbool_h = yes; then
     AC_DEFINE(HAVE_STDBOOL_H, 1, [Define to 1 if stdbool.h conforms to C99.])
   fi])
