#serial 1

dnl From Jim Meyering.
dnl FIXME
dnl

AC_DEFUN(jm_FUNC_NANOSLEEP,
[
 AC_CACHE_CHECK([whether nanosleep works],
  jm_cv_func_nanosleep_works,
  [AC_TRY_RUN([
#   include <time.h>

    int
    main ()
    {
      struct timespec ts_sleep, ts_remaining;
      ts_sleep.tv_sec = 0;
      ts_sleep.tv_nsec = 1;
      exit (nanosleep (&ts_sleep, &ts_remaining) == 0 ? 1 : 0);
    }
	  ],
	 jm_cv_func_nanosleep_works=yes,
	 jm_cv_func_nanosleep_works=no,
	 dnl When crosscompiling, assume the worst.
	 jm_cv_func_nanosleep_works=yes)
  ])
  if test $jm_cv_func_nanosleep_works = no; then
    AC_SUBST(LIBOBJS)
    LIBOBJS="$LIBOBJS nanosleep.$ac_objext"
    AC_DEFINE_UNQUOTED(nanosleep, gnu_nanosleep,
      [Define to gnu_nanosleep if the replacement function should be used.])
  fi
])
