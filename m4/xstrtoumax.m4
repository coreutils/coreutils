#serial 5
dnl Cloned from xstrtoimax.m4.  Keep these files in sync.

AC_DEFUN([jm_XSTRTOUMAX],
[
  dnl Prerequisites of lib/xstrtoumax.c.
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([gl_PREREQ_XSTRTOL])
])
