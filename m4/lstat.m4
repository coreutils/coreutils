#serial 1

dnl From Jim Meyering.
dnl Determine whether lstat has the bug that it succeeds when given the
dnl zero-length file name argument.  The lstat from SunOS4.1.4 does this.
dnl
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_lstat if the replacement function should be used.  */
dnl  #undef lstat
dnl

AC_DEFUN(jm_FUNC_LSTAT,
[
 AC_CACHE_CHECK([for working lstat], jm_cv_func_working_lstat,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (lstat ("", &sbuf) == 0 ? 1 : 0);
    }
	  ],
	 jm_cv_func_working_lstat=yes,
	 jm_cv_func_working_lstat=no,
	 dnl When crosscompiling, assume lstat is broken.
	 jm_cv_func_working_lstat=no)
  ])
  if test $jm_cv_func_working_lstat = no; then
    LIBOBJS="$LIBOBJS lstat.o"
    AC_DEFINE_UNQUOTED(lstat, rpl_lstat)
  fi
])
