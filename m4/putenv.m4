#serial 5

dnl From Jim Meyering.
dnl
dnl Check whether putenv ("FOO") removes FOO from the environment.
dnl The putenv in libc on at least SunOS 4.1.4 does *not* do that.
dnl

AC_DEFUN([jm_FUNC_PUTENV],
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
    AC_LIBOBJ(putenv)
    AC_DEFINE(putenv, rpl_putenv,
      [Define to rpl_putenv if the replacement function should be used.])
  fi
])
