#serial 2

dnl From Jim Meyering.
dnl Determine whether stat has the bug that it succeeds when given the
dnl zero-length file name argument.  The stat from SunOS4.1.4 and the Hurd
dnl (as of 1998-11-01) do this.
dnl
dnl If it does, then define HAVE_STAT_EMPTY_STRING_BUG and arrange to
dnl compile the wrapper function.
dnl

AC_DEFUN(jm_FUNC_STAT,
[
 AC_CACHE_CHECK([whether stat accepts an empty string],
  jm_cv_func_stat_empty_string_bug,
  [AC_TRY_RUN([
#   include <sys/types.h>
#   include <sys/stat.h>

    int
    main ()
    {
      struct stat sbuf;
      exit (stat ("", &sbuf) ? 1 : 0);
    }
	  ],
	 jm_cv_func_stat_empty_string_bug=yes,
	 jm_cv_func_stat_empty_string_bug=no,
	 dnl When crosscompiling, assume stat is broken.
	 jm_cv_func_stat_empty_string_bug=yes)
  ])
  if test $jm_cv_func_stat_empty_string_bug = yes; then

    LIBOBJS="$LIBOBJS stat.o"

    if test $jm_cv_func_stat_empty_string_bug = yes; then
      if test x = y; then
	# This code is deliberately never run via ./configure.
	# FIXME: this is a hack to make autoheader put the corresponding
	# HAVE_* undef for this symbol in config.h.in.  This saves me the
	# trouble of having to maintain the #undef in acconfig.h manually.
	AC_CHECK_FUNCS(STAT_EMPTY_STRING_BUG)
      fi
      # Defining it this way (rather than via AC_DEFINE) short-circuits the
      # autoheader check -- autoheader doesn't know it's already been taken
      # care of by the hack above.
      ac_kludge=HAVE_STAT_EMPTY_STRING_BUG
      AC_DEFINE_UNQUOTED($ac_kludge)
    fi
  fi
])
