#serial 1

dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl   /* Define to rpl_memcmp if the replacement function should be used.  */
dnl   #undef memcmp
dnl
AC_DEFUN(jm_FUNC_MEMCMP,
[AC_REQUIRE([AC_FUNC_MEMCMP])dnl
 if test $ac_cv_func_memcmp_clean = no; then
   AC_DEFINE_UNQUOTED(memcmp, rpl_memcmp)
 fi
])
