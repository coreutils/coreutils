#serial 1

dnl From Jim Meyering.
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_getgroups if the replacement function should be used.  */
dnl  #undef getgroups
dnl

AC_DEFUN(jm_FUNC_GETGROUPS,
[AC_REQUIRE([AC_TYPE_GETGROUPS])dnl
 AC_REQUIRE([AC_TYPE_SIZE_T])dnl
 AC_CHECK_FUNCS(getgroups)
 # Run the program to test the functionality of the system-supplied
 # getgroups function only if there is such a function.
 if test $ac_cv_func_getgroups = yes; then
   AC_CACHE_CHECK([for working getgroups], jm_cv_func_working_getgroups,
    [AC_TRY_RUN([
      int
      main ()
      {
	/* On Ultrix 4.3, getgroups (0, 0) always fails.  */
	exit (getgroups (0, 0) == -1 ? 1 : 0);
      }
		],
	       jm_cv_func_working_getgroups=yes,
	       jm_cv_func_working_getgroups=no,
	       dnl When crosscompiling, assume getgroups is broken.
	       jm_cv_func_working_getgroups=no)
    ])
    if test $jm_cv_func_working_getgroups = no; then
      LIBOBJS="$LIBOBJS getgroups.o"
      AC_DEFINE_UNQUOTED(getgroups, rpl_getgroups)
    fi
  fi
])
