#serial 7

dnl From Jim Meyering.
dnl Determine whether lstat has the bug that it succeeds when given the
dnl zero-length file name argument.  The lstat from SunOS4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_LSTAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN([jm_FUNC_LSTAT],
[
 AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
 AC_CACHE_CHECK([whether lstat accepts an empty string],
  jm_cv_func_lstat_empty_string_bug,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (lstat ("", &sbuf) ? 1 : 0);
    }
	  ],
	 jm_cv_func_lstat_empty_string_bug=yes,
	 jm_cv_func_lstat_empty_string_bug=no,
	 dnl When crosscompiling, assume lstat is broken.
	 jm_cv_func_lstat_empty_string_bug=yes)
  ])
  if test $jm_cv_func_lstat_empty_string_bug = yes; then
    AC_LIBOBJ(lstat)
    AC_DEFINE(HAVE_LSTAT_EMPTY_STRING_BUG, 1,
[Define if lstat has the bug that it succeeds when given the zero-length
   file name argument.  The lstat from SunOS4.1.4 and the Hurd as of 1998-11-01)
   do this. ])
  fi
])
