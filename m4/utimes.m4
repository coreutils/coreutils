#serial 2

dnl Shamelessly cloned from acspecific.m4's AC_FUNC_UTIME_NULL.

AC_DEFUN(jm_FUNC_UTIMES_NULL,
[AC_CACHE_CHECK(whether utimes accepts a null argument, ac_cv_func_utimes_null,
[rm -f conftestdata; > conftestdata
AC_TRY_RUN([
/* In case stat has been defined to rpl_stat, undef it here.  */
#undef stat
#include <sys/types.h>
#include <sys/stat.h>
main() {
struct stat s, t;
exit(!(stat ("conftestdata", &s) == 0 && utimes("conftestdata", (long *)0) == 0
&& stat("conftestdata", &t) == 0 && t.st_mtime >= s.st_mtime
&& t.st_mtime - s.st_mtime < 120));
}],
  ac_cv_func_utimes_null=yes,
  ac_cv_func_utimes_null=no,
  ac_cv_func_utimes_null=no)
rm -f core core.* *.core])

    if test $ac_cv_func_utimes_null = yes; then
      if test x = y; then
	# This code is deliberately never run via ./configure.
	# FIXME: this is a hack to make autoheader put the corresponding
	# HAVE_* undef for this symbol in config.h.in.  This saves me the
	# trouble of having to maintain the #undef in acconfig.h manually.
	AC_CHECK_FUNCS(UTIMES_NULL)
      fi
      # Defining it this way (rather than via AC_DEFINE) short-circuits the
      # autoheader check -- autoheader doesn't know it's already been taken
      # care of by the hack above.
      ac_kludge=HAVE_UTIMES_NULL
      AC_DEFINE_UNQUOTED($ac_kludge)
    fi
  ]
)
