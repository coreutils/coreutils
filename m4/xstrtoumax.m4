#serial 6
dnl Cloned from xstrtoimax.m4.  Keep these files in sync.

AC_DEFUN([gl_XSTRTOUMAX],
[
  dnl Prerequisites of lib/xstrtoumax.c.
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([gl_PREREQ_XSTRTOL])
])
