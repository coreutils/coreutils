#serial 3
dnl Cloned from xstrtoumax.m4.  Keep these files in sync.

AC_DEFUN([jm_XSTRTOIMAX],
[
  dnl Prerequisites of lib/xstrtoimax.c.
  AC_REQUIRE([jm_AC_TYPE_INTMAX_T])
  AC_REQUIRE([gl_PREREQ_XSTRTOL])
])
