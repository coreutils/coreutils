#serial 1

dnl From Jim Meyering.
dnl Determine whether stat has the bug that it succeeds when given the
dnl zero-length file name argument.  The stat from SunOS4.1.4 does this.
dnl
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_stat if the replacement function should be used.  */
dnl  #undef stat
dnl

AC_DEFUN(jm_FUNC_STAT,
[
 AC_CACHE_CHECK([for working stat], jm_cv_func_working_stat,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (stat ("", &sbuf) == 0 ? 1 : 0);
    }
	  ],
	 jm_cv_func_working_stat=yes,
	 jm_cv_func_working_stat=no,
	 dnl When crosscompiling, assume stat is broken.
	 jm_cv_func_working_stat=no)
  ])
  if test $jm_cv_func_working_stat = no; then
    LIBOBJS="$LIBOBJS stat.o"
    AC_DEFINE_UNQUOTED(stat, rpl_stat)
  fi
])
