#serial 2

# Determine whether calloc (N, S) returns non-NULL when N*S is zero.
# If so, define HAVE_CALLOC.  Otherwise, define calloc to rpl_calloc
# and arrange to use a calloc wrapper function that does work in that case.

# _AC_FUNC_CALLOC_IF(IF-WORKS, IF-NOT)
# -------------------------------------
# If `calloc (0, 0)' is properly handled, run IF-WORKS, otherwise, IF-NOT.
AC_DEFUN([_AC_FUNC_CALLOC_IF],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_CHECK_HEADERS(stdlib.h)
AC_CACHE_CHECK([for GNU libc compatible calloc], ac_cv_func_calloc_0_nonnull,
[AC_RUN_IFELSE(
[AC_LANG_PROGRAM(
[[#if STDC_HEADERS || HAVE_STDLIB_H
# include <stdlib.h>
#else
char *calloc ();
#endif
]],
		 [exit (calloc (0, 0) ? 0 : 1);])],
	       [ac_cv_func_calloc_0_nonnull=yes],
	       [ac_cv_func_calloc_0_nonnull=no],
	       [ac_cv_func_calloc_0_nonnull=no])])
AS_IF([test $ac_cv_func_calloc_0_nonnull = yes], [$1], [$2])
])# AC_FUNC_CALLOC


# AC_FUNC_CALLOC
# ---------------
# Report whether `calloc (0, 0)' is properly handled, and replace calloc if
# needed.
AC_DEFUN([AC_FUNC_CALLOC],
[_AC_FUNC_CALLOC_IF(
  [AC_DEFINE([HAVE_CALLOC], 1,
	     [Define to 1 if your system has a GNU libc compatible `calloc'
	      function, and to 0 otherwise.])],
  [AC_DEFINE([HAVE_CALLOC], 0)
   AC_LIBOBJ([calloc])
   AC_DEFINE([calloc], [rpl_calloc],
      [Define to rpl_calloc if the replacement function should be used.])])
])# AC_FUNC_CALLOC
