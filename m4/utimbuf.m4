#serial 1

dnl From Jim Meyering

dnl Define HAVE_STRUCT_UTIMBUF if `struct utimbuf' is declared --
dnl usually in <utime.h>.
dnl Some systems have utime.h but don't declare the struct anywhere.

AC_DEFUN(jm_STRUCT_UTIMBUF,
[
  AC_CHECK_HEADERS(utime.h)
  AC_REQUIRE([AC_HEADER_TIME])
  AC_CACHE_CHECK([for struct utimbuf], fu_cv_sys_struct_utimbuf,
    [AC_TRY_COMPILE(
      [
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
      ],
      [static struct utimbuf x; x.actime = x.modtime;],
      fu_cv_sys_struct_utimbuf=yes,
      fu_cv_sys_struct_utimbuf=no)
    ])

  if test $fu_cv_sys_struct_utimbuf = yes; then
    if test x = y; then
      # This code is deliberately never run via ./configure.
      # FIXME: this is a hack to make autoheader put the corresponding
      # HAVE_* undef for this symbol in config.h.in.  This saves me the
      # trouble of having to maintain the #undef in acconfig.h manually.
      AC_CHECK_FUNCS(STRUCT_UTIMBUF)
    fi
    # Defining it this way (rather than via AC_DEFINE) short-circuits the
    # autoheader check -- autoheader doesn't know it's already been taken
    # care of by the hack above.
    ac_kludge=HAVE_STRUCT_UTIMBUF
    AC_DEFINE_UNQUOTED($ac_kludge)
  fi
])
