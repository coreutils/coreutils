#serial 5

dnl From Jim Meyering.
dnl
dnl Invoking code should check $GETGROUPS_LIB something like this:
dnl  jm_FUNC_GETGROUPS
dnl  test -n "$GETGROUPS_LIB" && LIBS="$GETGROUPS_LIB $LIBS"
dnl

AC_DEFUN([jm_FUNC_GETGROUPS],
[AC_REQUIRE([AC_TYPE_GETGROUPS])dnl
 AC_REQUIRE([AC_TYPE_SIZE_T])dnl
 AC_CHECK_FUNCS(getgroups)

 # If we don't yet have getgroups, see if it's in -lbsd.
 # This is reported to be necessary on an ITOS 3000WS running SEIUX 3.1.
 if test $ac_cv_func_getgroups = no; then
   jm_cv_sys_getgroups_saved_lib="$LIBS"
   AC_CHECK_LIB(bsd, getgroups, [GETGROUPS_LIB=-lbsd])
   LIBS="$jm_cv_sys_getgroups_saved_lib"
 fi

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
      AC_LIBOBJ(getgroups)
      AC_DEFINE(getgroups, rpl_getgroups,
	[Define as rpl_getgroups if getgroups doesn't work right.])
    fi
  fi
])
