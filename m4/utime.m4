#serial 3

dnl From Jim Meyering
dnl Replace the utime function on systems that need it.

dnl FIXME

AC_DEFUN([jm_FUNC_UTIME],
[
  AC_CHECK_HEADERS(utime.h)
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_UTIMBUF])
  AC_REQUIRE([AC_FUNC_UTIME_NULL])

  if test $ac_cv_func_utime_null = no; then
    jm_FUNC_UTIMES_NULL
    AC_REPLACE_FUNCS(utime)
  fi
])
