#serial 1

dnl From Jim Meyering.
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_putenv if the replacement function should be used.  */
dnl  #undef putenv
dnl

AC_DEFUN(jm_FUNC_PUTENV,
[AC_CACHE_CHECK([for SVID conformant putenv], jm_cv_func_svid_putenv,
  [AC_TRY_RUN([
    int
    main ()
    {
      /* Put it in env.  */
      if (putenv ("CONFTEST_putenv=val"))
        exit (1);

      /* Try to remove it.  */
      if (putenv ("CONFTEST_putenv"))
        exit (1);

      /* Make sure it was deleted.  */
      if (getenv ("CONFTEST_putenv") != 0)
        exit (1);

      exit (0);
    }
	      ],
	     jm_cv_func_svid_putenv=yes,
	     jm_cv_func_svid_putenv=no,
	     dnl When crosscompiling, assume putenv is broken.
	     jm_cv_func_svid_putenv=no)
  ])
  if test $jm_cv_func_svid_putenv = no; then
    LIBOBJS="$LIBOBJS putenv.o"
    AC_DEFINE_UNQUOTED(putenv, rpl_putenv)
  fi
])
