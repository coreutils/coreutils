#serial 21

dnl This macro is intended to be used solely in this file.
dnl These are the prerequisite macros for GNU's strftime.c replacement.
AC_DEFUN([_jm_STRFTIME_PREREQS],
[
 dnl strftime.c uses the underyling system strftime if it exists.
 AC_FUNC_STRFTIME

 AC_CHECK_FUNCS_ONCE(mempcpy)
 AC_CHECK_FUNCS(tzset)

 # This defines (or not) HAVE_TZNAME and HAVE_TM_ZONE.
 AC_STRUCT_TIMEZONE

 AC_CHECK_FUNCS(mblen mbrlen)
 AC_TYPE_MBSTATE_T

 AC_REQUIRE([gl_TM_GMTOFF])
 AC_REQUIRE([gl_FUNC_TZSET_CLOBBER])
])

dnl From Jim Meyering.
dnl
AC_DEFUN([jm_FUNC_GNU_STRFTIME],
[AC_REQUIRE([AC_HEADER_TIME])dnl

 _jm_STRFTIME_PREREQS

 AC_REQUIRE([AC_C_CONST])dnl
 AC_CHECK_HEADERS_ONCE(sys/time.h)
 AC_DEFINE([my_strftime], [nstrftime],
   [Define to the name of the strftime replacement function.])
])

AC_DEFUN([jm_FUNC_STRFTIME],
[
  _jm_STRFTIME_PREREQS
])
