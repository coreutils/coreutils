# putenv.m4 serial 11
dnl Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.
dnl
dnl Check whether putenv ("FOO") removes FOO from the environment.
dnl The putenv in libc on at least SunOS 4.1.4 does *not* do that.

AC_DEFUN([gl_FUNC_PUTENV],
[AC_CACHE_CHECK([for SVID conformant putenv], jm_cv_func_svid_putenv,
  [AC_TRY_RUN([
    int
    main ()
    {
      /* Put it in env.  */
      if (putenv ("CONFTEST_putenv=val"))
        return 1;

      /* Try to remove it.  */
      if (putenv ("CONFTEST_putenv"))
        return 1;

      /* Make sure it was deleted.  */
      if (getenv ("CONFTEST_putenv") != 0)
        return 1;

      return 0;
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
