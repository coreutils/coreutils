#serial 5

dnl From Jim Meyering
dnl Replace the utime function on systems that need it.

dnl FIXME

AC_DEFUN([jm_FUNC_UTIME],
[
  AC_REQUIRE([AC_FUNC_UTIME_NULL])
  if test $ac_cv_func_utime_null = no; then
    AC_LIBOBJ(utime)
    AC_DEFINE(utime, rpl_utime,
      [Define to rpl_utime if the replacement function should be used.])
    gl_PREREQ_UTIME
  fi
])

# Prerequisites of lib/utime.c.
AC_DEFUN([gl_PREREQ_UTIME],
[
  AC_CHECK_HEADERS_ONCE(utime.h)
  AC_REQUIRE([jm_CHECK_TYPE_STRUCT_UTIMBUF])
  jm_FUNC_UTIMES_NULL
])
