#serial 3

dnl From Jim Meyering.
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl   /* Define to rpl_mktime if the replacement function should be used.  */
dnl   #undef mktime
dnl
AC_DEFUN(jm_FUNC_MKTIME,
[AC_REQUIRE([jm_AM_FUNC_MKTIME])dnl

 dnl mktime.c uses localtime_r if it exists.  Check for it.
 AC_CHECK_FUNCS(localtime_r)

 if test $jm_am_cv_func_working_mktime = no; then
   AC_DEFINE_UNQUOTED(mktime, rpl_mktime)
 fi
])
