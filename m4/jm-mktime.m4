#serial 5

dnl From Jim Meyering.
dnl A wrapper around AM_FUNC_MKTIME.

AC_DEFUN(jm_FUNC_MKTIME,
[AC_REQUIRE([AM_FUNC_MKTIME])dnl

 dnl mktime.c uses localtime_r if it exists.  Check for it.
 AC_CHECK_FUNCS(localtime_r)

 if test $am_cv_func_working_mktime = no; then
   AC_DEFINE_UNQUOTED(mktime, rpl_mktime,
    [Define to gnu_mktime if the replacement function should be used.])
 fi
])
