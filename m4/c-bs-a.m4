#serial 4

dnl From Paul Eggert.

AC_DEFUN([AC_C_BACKSLASH_A],
[
  AC_CACHE_CHECK([whether backslash-a works in strings], ac_cv_c_backslash_a,
   [AC_TRY_COMPILE([],
     [
#if '\a' == 'a'
      syntax error;
#endif
      char buf['\a' == 'a' ? -1 : 1];
      buf[0] = '\a';
      return buf[0] != "\a"[0];
     ],
     ac_cv_c_backslash_a=yes,
     ac_cv_c_backslash_a=no)])
  if test $ac_cv_c_backslash_a = yes; then
    AC_DEFINE(HAVE_C_BACKSLASH_A, 1,
      [Define if backslash-a works in C strings.])
  fi
])
