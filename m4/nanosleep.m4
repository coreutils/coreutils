#serial 2

dnl From Jim Meyering.
dnl Check for the nanosleep function
dnl

AC_DEFUN(jm_FUNC_NANOSLEEP,
[
 # Solaris 2.5.1 needs -lposix4 to get the nanosleep function.
 # Solaris 7 prefers the library name -lrt to the obsolescent name -lposix4.
 AC_SEARCH_LIBS(nanosleep, [rt posix4])

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
      exit (nanosleep (&ts_sleep, &ts_remaining) == 0 ? 0 : 1);
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
